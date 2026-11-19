import { Crepe } from '@milkdown/crepe'
import '@milkdown/crepe/theme/common/style.css'
import '@milkdown/crepe/theme/frame.css'
import { replaceAll } from '@milkdown/kit/utils'
import './style.css'

interface DocumentBridge {
  editorReady(): void
  contentChanged(markdown: string, revision: number): void
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
let applyingFromQt = false
let activeRevision = 0
let ignoredMarkdown: string | null = null

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
    if (applyingFromQt || markdown === previousMarkdown) {
      return
    }
    if (markdown === ignoredMarkdown) {
      ignoredMarkdown = null
      return
    }
    ignoredMarkdown = null
    bridge.contentChanged(markdown, activeRevision)
  })
})

await crepe.create()

bridge.setMarkdownRequested.connect((markdown, revision) => {
  activeRevision = revision
  applyingFromQt = true
  crepe.editor.action(replaceAll(markdown, true))
  ignoredMarkdown = crepe.getMarkdown()
  applyingFromQt = false
  crepe.setReadonly(false)
})

bridge.editorReady()
