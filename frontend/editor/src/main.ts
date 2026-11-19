import { Crepe } from '@milkdown/crepe'
import '@milkdown/crepe/theme/common/style.css'
import '@milkdown/crepe/theme/frame.css'
import { replaceAll } from '@milkdown/kit/utils'
import './style.css'

interface DocumentBridge {
  editorReady(): void
  contentChanged(markdown: string, revision: number): void
  documentApplied(revision: number): void
  setMarkdownRequested: {
    connect(callback: (markdown: string, revision: number) => void): void
  }
}

interface WebChannelObjects {
  documentBridge: DocumentBridge
}

declare global {
  interface Window {
    qt: { webChannelTransport: unknown }
    QWebChannel: new (
      transport: unknown,
      callback: (channel: { objects: WebChannelObjects }) => void,
    ) => unknown
  }
}

const connectToQt = () =>
  new Promise<DocumentBridge>((resolve) => {
    new window.QWebChannel(window.qt.webChannelTransport, (channel) => {
      resolve(channel.objects.documentBridge)
    })
  })

const bridge = await connectToQt()
let activeRevision = 0
let pendingDocumentChange: { markdown: string; revision: number } | undefined
let documentChangeTimer: number | undefined

const imageToDataUrl = (file: File) =>
  new Promise<string>((resolve, reject) => {
    const reader = new FileReader()
    reader.addEventListener('load', () => resolve(reader.result as string))
    reader.addEventListener('error', () => reject(reader.error))
    reader.readAsDataURL(file)
  })

const crepe = new Crepe({
  root: '#editor',
  defaultValue: '',
  features: {
    [Crepe.Feature.TopBar]: true,
    [Crepe.Feature.AI]: false,
  },
  featureConfigs: {
    [Crepe.Feature.ImageBlock]: {
      onUpload: imageToDataUrl,
    },
    [Crepe.Feature.Placeholder]: {
      text: 'Dokument bearbeiten …',
      mode: 'block',
    },
  },
})

crepe.setReadonly(true)
crepe.on((listener) => {
  listener.markdownUpdated((_ctx, markdown, previousMarkdown) => {
    if (markdown === previousMarkdown) {
      return
    }
    bridge.contentChanged(markdown, activeRevision)
  })
})

await crepe.create()

bridge.setMarkdownRequested.connect((markdown, revision) => {
  pendingDocumentChange = { markdown, revision }
  window.clearTimeout(documentChangeTimer)
  documentChangeTimer = window.setTimeout(() => {
    const documentChange = pendingDocumentChange
    pendingDocumentChange = undefined
    if (!documentChange) return

    activeRevision = documentChange.revision
    crepe.editor.action(replaceAll(documentChange.markdown, true))
    crepe.setReadonly(false)
    window.requestAnimationFrame(() => {
      window.requestAnimationFrame(() => {
        bridge.documentApplied(documentChange.revision)
      })
    })
  }, 0)
})

bridge.editorReady()
