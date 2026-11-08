import {
  useCallback,
  useEffect,
  useRef,
  useState,
  type MutableRefObject,
} from 'react'
import {
  Background,
  ReactFlow,
  useEdgesState,
  useNodesState,
  type Edge,
  type Node,
} from '@xyflow/react'
import '@xyflow/react/dist/style.css'
import {
  BlockTypeSelect,
  BoldItalicUnderlineToggles,
  CodeToggle,
  CreateLink,
  InsertCodeBlock,
  InsertTable,
  ListsToggle,
  MDXEditor,
  Separator,
  UndoRedo,
  codeBlockPlugin,
  codeMirrorPlugin,
  headingsPlugin,
  linkDialogPlugin,
  linkPlugin,
  listsPlugin,
  markdownShortcutPlugin,
  tablePlugin,
  toolbarPlugin,
} from '@mdxeditor/editor'
import '@mdxeditor/editor/style.css'
import dagre from '@dagrejs/dagre'
import {
  Alert,
  Box,
  Button,
  Center,
  Group,
  Loader,
  Modal,
  Paper,
  Stack,
  Text,
  TextInput,
} from '@mantine/core'
import { useForm } from '@mantine/form'
import { useQueryClient } from '@tanstack/react-query'
import {
  topicGraphKey,
  updateDocumentContent,
  useCreateTopic,
  useTopicDocument,
  useTopicGraph,
} from './api'
import type { Document, Topic } from './types'

interface TopicNodeData extends Record<string, unknown> {
  document: Document | null
  label: string
  topic: Topic
}

type TopicNode = Node<TopicNodeData>

interface ContextMenuPosition {
  x: number
  y: number
  topic: TopicNode | null
}

interface ContextMenuEvent {
  clientX: number
  clientY: number
  preventDefault: () => void
  stopPropagation: () => void
}

interface TopicFormValues {
  name: string
}

type SaveStatus = 'saved' | 'saving' | 'error'

const editorPlugins = [
  headingsPlugin(),
  listsPlugin(),
  tablePlugin(),
  linkPlugin(),
  linkDialogPlugin(),
  codeBlockPlugin({ defaultCodeBlockLanguage: 'txt' }),
  codeMirrorPlugin({
    codeBlockLanguages: {
      css: 'CSS',
      html: 'HTML',
      js: 'JavaScript',
      md: 'Markdown',
      rust: 'Rust',
      ts: 'TypeScript',
      txt: 'Text',
    },
  }),
  markdownShortcutPlugin(),
  toolbarPlugin({
    toolbarContents: () => (
      <>
        <UndoRedo />
        <Separator />
        <BlockTypeSelect />
        <BoldItalicUnderlineToggles />
        <CodeToggle />
        <Separator />
        <ListsToggle />
        <CreateLink />
        <InsertTable />
        <InsertCodeBlock />
      </>
    ),
  }),
]

function applyDagreLayout(nodes: TopicNode[], edges: Edge[]) {
  const graph = new dagre.graphlib.Graph()
  graph.setDefaultEdgeLabel(() => ({}))
  graph.setGraph({
    edgesep: 8,
    marginx: 10,
    marginy: 10,
    nodesep: 20,
    rankdir: 'TB',
    ranker: 'tight-tree',
    ranksep: 30,
  })

  nodes.forEach((node) => graph.setNode(node.id, { width: 180, height: 50 }))
  edges.forEach((edge) => graph.setEdge(edge.source, edge.target))
  dagre.layout(graph)

  return nodes.map((node) => {
    const position = graph.node(node.id)
    return {
      ...node,
      position: { x: position.x - 90, y: position.y - 25 },
    }
  })
}

function LoadedDocumentPanel({
  topic,
  document,
  drafts,
}: {
  topic: Topic
  document: Document
  drafts: MutableRefObject<Map<number, string>>
}) {
  const queryClient = useQueryClient()
  const initialMarkdown = drafts.current.get(topic.entity_id) ?? document.content
  const [saveStatus, setSaveStatus] = useState<SaveStatus>(
    drafts.current.has(topic.entity_id) ? 'error' : 'saved',
  )
  const draftRef = useRef(initialMarkdown)
  const savedRef = useRef(document.content)
  const timerRef = useRef<ReturnType<typeof setTimeout> | null>(null)
  const saveQueueRef = useRef<Promise<void>>(Promise.resolve())
  const mountedRef = useRef(true)

  const queueSave = useCallback((content: string, showStatus: boolean) => {
    if (content === savedRef.current) {
      if (showStatus && mountedRef.current) setSaveStatus('saved')
      return
    }

    if (showStatus && mountedRef.current) setSaveStatus('saving')
    saveQueueRef.current = saveQueueRef.current
      .catch(() => undefined)
      .then(async () => {
        if (content === savedRef.current) return

        await updateDocumentContent(document.entity_id, content)
        savedRef.current = content
        if (drafts.current.get(topic.entity_id) === content) {
          drafts.current.delete(topic.entity_id)
        }
        await queryClient.invalidateQueries({
          queryKey: topicGraphKey,
          refetchType: 'none',
        })
        if (
          showStatus &&
          mountedRef.current &&
          draftRef.current === content
        ) {
          setSaveStatus('saved')
        }
      })
      .catch(() => {
        if (showStatus && mountedRef.current) setSaveStatus('error')
      })
  }, [document.entity_id, drafts, queryClient, topic.entity_id])

  useEffect(() => {
    mountedRef.current = true
    return () => {
      mountedRef.current = false
      if (timerRef.current !== null) clearTimeout(timerRef.current)
      queueSave(draftRef.current, false)
    }
  }, [queueSave])

  const handleChange = (markdown: string, initialMarkdownNormalize: boolean) => {
    if (initialMarkdownNormalize) {
      draftRef.current = markdown
      return
    }

    draftRef.current = markdown
    drafts.current.set(topic.entity_id, markdown)
    setSaveStatus('saving')
    if (timerRef.current !== null) clearTimeout(timerRef.current)
    timerRef.current = setTimeout(() => queueSave(markdown, true), 1_500)
  }

  const statusLabel =
    saveStatus === 'saved'
      ? 'Gespeichert'
      : saveStatus === 'saving'
        ? 'Speichert …'
        : 'Fehler'

  return (
    <Paper
      bottom={0}
      className="realization-document-panel"
      pos="absolute"
      radius={0}
      right={0}
      top={0}
      w={520}
      withBorder
      style={{ display: 'flex', flexDirection: 'column', zIndex: 5 }}
    >
      <Group justify="space-between" p="sm">
        <Text fw={600}>{topic.name}</Text>
        <Text c={saveStatus === 'error' ? 'red' : 'dimmed'} size="sm">
          {statusLabel}
        </Text>
      </Group>
      <Box style={{ flex: 1, minHeight: 0, overflow: 'auto' }}>
        <MDXEditor
          key={topic.entity_id}
          markdown={initialMarkdown}
          plugins={editorPlugins}
          onChange={handleChange}
        />
      </Box>
    </Paper>
  )
}

function TopicDocumentPanel({
  topic,
  document,
  drafts,
}: {
  topic: Topic
  document: Document | null
  drafts: MutableRefObject<Map<number, string>>
}) {
  const topicDocument = useTopicDocument(
    topic.entity_id,
    document?.entity_id ?? null,
  )

  if (topicDocument.isPending) {
    return (
      <Paper bottom={0} pos="absolute" radius={0} right={0} top={0} w={520} withBorder style={{ zIndex: 5 }}>
        <Center h="100%"><Loader /></Center>
      </Paper>
    )
  }

  if (topicDocument.isError) {
    return (
      <Paper bottom={0} p="md" pos="absolute" radius={0} right={0} top={0} w={520} withBorder style={{ zIndex: 5 }}>
        <Alert color="red" title="Fehler">
          Das Dokument konnte nicht geladen oder erstellt werden.
        </Alert>
      </Paper>
    )
  }

  return (
    <LoadedDocumentPanel
      document={topicDocument.data}
      drafts={drafts}
      topic={topic}
    />
  )
}

export function RealizationView() {
  const topicGraph = useTopicGraph()
  const createTopic = useCreateTopic()
  const [nodes, setNodes, onNodesChange] = useNodesState<TopicNode>([])
  const [edges, setEdges, onEdgesChange] = useEdgesState<Edge>([])
  const [contextMenu, setContextMenu] = useState<ContextMenuPosition | null>(null)
  const [parentTopic, setParentTopic] = useState<TopicNode | null>(null)
  const [selectedTopicId, setSelectedTopicId] = useState<string | null>(null)
  const [createOpened, setCreateOpened] = useState(false)
  const flowRef = useRef<HTMLDivElement>(null)
  const documentDraftsRef = useRef(new Map<number, string>())
  const form = useForm<TopicFormValues>({
    initialValues: { name: '' },
    validate: {
      name: (value) => value.trim() === '' ? 'Name ist erforderlich.' : null,
    },
  })
  const selectedTopic = nodes.find((node) => node.id === selectedTopicId)

  useEffect(() => {
    if (topicGraph.data === undefined) return

    const nextNodes: TopicNode[] = topicGraph.data.map(({ topic, document }) => ({
      id: String(topic.entity_id),
      data: { document, label: topic.name, topic },
      position: { x: 0, y: 0 },
    }))
    const nodeIds = new Set(nextNodes.map((node) => node.id))
    const nextEdges: Edge[] = topicGraph.data.flatMap(({ topic }) =>
      topic.parent_topic_entity_id === null ||
      !nodeIds.has(String(topic.parent_topic_entity_id))
        ? []
        : [{
            id: `${topic.parent_topic_entity_id}-${topic.entity_id}`,
            source: String(topic.parent_topic_entity_id),
            target: String(topic.entity_id),
          }],
    )

    setEdges(nextEdges)
    setNodes(applyDagreLayout(nextNodes, nextEdges))
  }, [setEdges, setNodes, topicGraph.data])

  const openContextMenu = (event: ContextMenuEvent, topic: TopicNode | null) => {
    event.preventDefault()
    event.stopPropagation()
    const bounds = flowRef.current?.getBoundingClientRect()
    if (bounds === undefined) return

    setContextMenu({
      x: event.clientX - bounds.left,
      y: event.clientY - bounds.top,
      topic,
    })
  }

  const openCreate = () => {
    setParentTopic(contextMenu?.topic ?? null)
    setContextMenu(null)
    setCreateOpened(true)
  }

  const closeCreate = () => {
    createTopic.reset()
    form.reset()
    setParentTopic(null)
    setCreateOpened(false)
  }

  const submit = (values: TopicFormValues) => {
    const name = values.name.trim()
    createTopic.mutate(
      {
        parent_topic_entity_id:
          parentTopic === null ? null : Number(parentTopic.id),
        name,
      },
      {
        onSuccess: closeCreate,
      },
    )
  }

  return (
    <Box ref={flowRef} h="100%" pos="relative" w="100%">
      <ReactFlow
        fitView
        edges={edges}
        nodes={nodes}
        onEdgesChange={onEdgesChange}
        onNodeClick={(_, node) => setSelectedTopicId(node.id)}
        onNodeContextMenu={(event, node) => openContextMenu(event, node)}
        onNodesChange={onNodesChange}
        onPaneClick={() => setContextMenu(null)}
        onPaneContextMenu={(event) => openContextMenu(event, null)}
      >
        <Background />
      </ReactFlow>

      {selectedTopic !== undefined && (
        <TopicDocumentPanel
          key={selectedTopic.id}
          document={selectedTopic.data.document}
          drafts={documentDraftsRef}
          topic={selectedTopic.data.topic}
        />
      )}

      {contextMenu !== null && (
        <Paper
          p={4}
          shadow="md"
          style={{
            left: contextMenu.x,
            position: 'absolute',
            top: contextMenu.y,
            zIndex: 10,
          }}
          withBorder
        >
          {contextMenu.topic !== null && (
            <Text fw={600} px="xs" py={4} size="sm">
              {contextMenu.topic.data.label}
            </Text>
          )}
          <Button size="compact-sm" variant="subtle" onClick={openCreate}>
            Add Topic
          </Button>
        </Paper>
      )}

      <Modal centered opened={createOpened} title="Topic erstellen" onClose={closeCreate}>
        <Box component="form" onSubmit={form.onSubmit(submit)}>
          <Stack>
            <TextInput
              data-autofocus
              required
              label="Name"
              {...form.getInputProps('name')}
            />
            {createTopic.isError && (
              <Alert color="red" title="Fehler">
                Das Topic konnte nicht erstellt werden.
              </Alert>
            )}
            <Group justify="flex-end">
              <Button disabled={createTopic.isPending} type="button" variant="default" onClick={closeCreate}>
                Abbrechen
              </Button>
              <Button loading={createTopic.isPending} type="submit">
                Erstellen
              </Button>
            </Group>
          </Stack>
        </Box>
      </Modal>
    </Box>
  )
}
