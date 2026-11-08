import {
  useCallback,
  useEffect,
  useRef,
  useState,
} from 'react'
import {
  Background,
  ReactFlow,
  useEdgesState,
  useNodesState,
  type Edge,
  type Node,
  type ReactFlowInstance,
} from '@xyflow/react'
import '@xyflow/react/dist/style.css'
import { Crepe } from '@milkdown/crepe'
import '@milkdown/crepe/theme/common/style.css'
import '@milkdown/crepe/theme/frame.css'
import ELK from 'elkjs/lib/elk.bundled.js'
import {
  Alert,
  ActionIcon,
  Box,
  Button,
  Center,
  Group,
  Loader,
  Modal,
  Paper,
  Select,
  SimpleGrid,
  Stack,
  Switch,
  Text,
  TextInput,
  Textarea,
} from '@mantine/core'
import { useForm } from '@mantine/form'
import { useQueryClient } from '@tanstack/react-query'
import {
  IconCards,
  IconPencil,
  IconPlus,
  IconTrash,
  IconX,
} from '@tabler/icons-react'
import { Group as PanelGroup, Panel, Separator as PanelSeparator } from 'react-resizable-panels'
import { useMetadata } from '../metadata/api'
import {
  topicGraphKey,
  updateDocumentContent,
  useCreateFlashcard,
  useCreateTopic,
  useDeleteFlashcard,
  useTopicDocument,
  useTopicGraph,
  useUpdateFlashcard,
} from './api'
import type {
  CreateFlashcard,
  Document,
  Flashcard,
  Topic,
  UpdateFlashcard,
} from './types'
import './RealizationView.css'

interface TopicNodeData extends Record<string, unknown> {
  document: Document | null
  flashcards: Flashcard[]
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

interface FlashcardFormValues {
  back: string
  explanation: string
  front: string
  hint: string
  is_enabled: boolean
  retention_levels_id: string | null
}

type SaveStatus = 'saved' | 'saving' | 'error'

const elk = new ELK()
const layoutOptions = {
  'elk.algorithm': 'layered',
  'elk.direction': 'DOWN',
  'elk.spacing.nodeNode': '15',
  'elk.layered.spacing.nodeNodeBetweenLayers': '30',
  'elk.layered.nodePlacement.strategy': 'NETWORK_SIMPLEX',
  'elk.edgeRouting': 'SPLINES',
}

interface HierarchyLayout {
  height: number
  nodes: TopicNode[]
  width: number
}

const hierarchyGap = 40

async function layoutHierarchy(
  nodes: TopicNode[],
  edges: Edge[],
): Promise<HierarchyLayout> {
  const graph = await elk.layout({
    id: 'root',
    layoutOptions,
    children: nodes.map((node) => ({
      id: node.id,
      height: node.measured?.height ?? node.height ?? 50,
      width: node.measured?.width ?? node.width ?? 180,
    })),
    edges: edges.map((edge) => ({
      id: edge.id,
      sources: [edge.source],
      targets: [edge.target],
    })),
  })
  const layoutNodes = graph.children ?? []
  const nodeSizes = new Map(nodes.map((node) => [
    node.id,
    {
      height: node.measured?.height ?? node.height ?? 50,
      width: node.measured?.width ?? node.width ?? 180,
    },
  ]))
  const minX = Math.min(...layoutNodes.map((node) => node.x ?? 0))
  const minY = Math.min(...layoutNodes.map((node) => node.y ?? 0))
  const maxX = Math.max(...layoutNodes.map((node) =>
    (node.x ?? 0) + (nodeSizes.get(node.id)?.width ?? 0),
  ))
  const maxY = Math.max(...layoutNodes.map((node) =>
    (node.y ?? 0) + (nodeSizes.get(node.id)?.height ?? 0),
  ))
  const positions = new Map(layoutNodes.map((node) => [
    node.id,
    { x: (node.x ?? 0) - minX, y: (node.y ?? 0) - minY },
  ]))

  return {
    height: maxY - minY,
    nodes: nodes.map((node) => ({
      ...node,
      position: positions.get(node.id) ?? node.position,
    })),
    width: maxX - minX,
  }
}

async function applyElkLayout(
  nodes: TopicNode[],
  edges: Edge[],
  availableWidth: number,
  availableHeight: number,
) {
  const outgoingEdges = new Map<string, Edge[]>()
  edges.forEach((edge) => {
    const outgoing = outgoingEdges.get(edge.source) ?? []
    outgoing.push(edge)
    outgoingEdges.set(edge.source, outgoing)
  })

  const assignedNodeIds = new Set<string>()
  const collectHierarchy = (rootId: string) => {
    const hierarchyNodeIds = new Set<string>()
    const pendingNodeIds = [rootId]

    while (pendingNodeIds.length > 0) {
      const nodeId = pendingNodeIds.pop()
      if (
        nodeId === undefined ||
        assignedNodeIds.has(nodeId) ||
        hierarchyNodeIds.has(nodeId)
      ) continue

      hierarchyNodeIds.add(nodeId)
      assignedNodeIds.add(nodeId)
      outgoingEdges.get(nodeId)?.forEach((edge) => {
        pendingNodeIds.push(edge.target)
      })
    }

    return nodes.filter((node) => hierarchyNodeIds.has(node.id))
  }

  const rootNodes = nodes.filter(
    (node) => node.data.topic.parent_topic_entity_id === null,
  )
  const hierarchies = rootNodes.map((rootNode) => collectHierarchy(rootNode.id))
  nodes.forEach((node) => {
    if (!assignedNodeIds.has(node.id)) {
      hierarchies.push(collectHierarchy(node.id))
    }
  })

  const layouts = await Promise.all(hierarchies.map((hierarchyNodes) => {
    const hierarchyNodeIds = new Set(hierarchyNodes.map((node) => node.id))
    const hierarchyEdges = edges.filter((edge) =>
      hierarchyNodeIds.has(edge.source) && hierarchyNodeIds.has(edge.target),
    )
    return layoutHierarchy(hierarchyNodes, hierarchyEdges)
  }))
  layouts.sort((left, right) => right.height - left.height)

  const totalArea = layouts.reduce(
    (area, layout) =>
      area +
      (layout.width + hierarchyGap) * (layout.height + hierarchyGap),
    0,
  )
  const widestHierarchy = Math.max(...layouts.map((layout) => layout.width))
  const allHierarchiesWidth = layouts.reduce(
    (width, layout) => width + layout.width,
    hierarchyGap * Math.max(0, layouts.length - 1),
  )
  const panelAspectRatio = Math.max(
    0.5,
    Math.min(2.5, availableWidth / Math.max(availableHeight, 1)),
  )
  const targetWidth = Math.max(
    widestHierarchy,
    Math.min(
      allHierarchiesWidth,
      Math.max(availableWidth, Math.sqrt(totalArea * panelAspectRatio)),
    ),
  )

  let x = 0
  let y = 0
  let rowHeight = 0
  const positionedNodes: TopicNode[] = []
  layouts.forEach((layout) => {
    if (x > 0 && x + layout.width > targetWidth) {
      x = 0
      y += rowHeight + hierarchyGap
      rowHeight = 0
    }

    positionedNodes.push(...layout.nodes.map((node) => ({
      ...node,
      position: {
        x: node.position.x + x,
        y: node.position.y + y,
      },
    })))
    x += layout.width + hierarchyGap
    rowHeight = Math.max(rowHeight, layout.height)
  })

  return positionedNodes
}

function CrepeMarkdownEditor({
  markdown,
  onChange,
}: {
  markdown: string
  onChange: (markdown: string) => void
}) {
  const rootRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    if (rootRef.current === null) return

    const crepe = new Crepe({
      defaultValue: markdown,
      features: { [Crepe.Feature.TopBar]: true },
      root: rootRef.current,
    })
    crepe.on((listener) => {
      listener.markdownUpdated((_, nextMarkdown) => onChange(nextMarkdown))
    })
    const createPromise = crepe.create()

    return () => {
      void createPromise.then(() => crepe.destroy())
    }
  }, [markdown, onChange])

  return <div ref={rootRef} className="realization-crepe" />
}

function FlashcardForm({
  documentId,
  flashcard,
  onClose,
}: {
  documentId: number
  flashcard: Flashcard | null
  onClose: () => void
}) {
  const metadata = useMetadata()
  const createFlashcard = useCreateFlashcard()
  const updateFlashcard = useUpdateFlashcard()
  const form = useForm<FlashcardFormValues>({
    initialValues: {
      back: flashcard?.back ?? '',
      explanation: flashcard?.explanation ?? '',
      front: flashcard?.front ?? '',
      hint: flashcard?.hint ?? '',
      is_enabled: flashcard?.is_enabled ?? true,
      retention_levels_id: String(flashcard?.retention_levels_id ?? 1),
    },
    validate: {
      back: (value) => value.trim() === '' ? 'Rückseite ist erforderlich.' : null,
      front: (value) => value.trim() === '' ? 'Vorderseite ist erforderlich.' : null,
      retention_levels_id: (value) =>
        value === null ? 'Erinnerungsstufe ist erforderlich.' : null,
    },
  })
  const isSaving = createFlashcard.isPending || updateFlashcard.isPending

  const submit = (values: FlashcardFormValues) => {
    const card: UpdateFlashcard = {
      back: values.back.trim(),
      explanation: values.explanation.trim() || null,
      front: values.front.trim(),
      hint: values.hint.trim() || null,
      is_enabled: values.is_enabled,
      retention_levels_id: Number(values.retention_levels_id),
    }

    if (flashcard === null) {
      const newCard: CreateFlashcard = { ...card, document_id: documentId }
      createFlashcard.mutate(newCard, { onSuccess: onClose })
      return
    }

    updateFlashcard.mutate(
      { flashcard: card, id: flashcard.id },
      { onSuccess: onClose },
    )
  }

  return (
    <Box component="form" onSubmit={form.onSubmit(submit)}>
      <Stack gap="sm">
        <Textarea
          autosize
          data-autofocus
          required
          label="Vorderseite"
          minRows={2}
          {...form.getInputProps('front')}
        />
        <Textarea
          autosize
          required
          label="Rückseite"
          minRows={2}
          {...form.getInputProps('back')}
        />
        <Textarea
          autosize
          label="Hinweis"
          minRows={2}
          {...form.getInputProps('hint')}
        />
        <Textarea
          autosize
          label="Erklärung"
          minRows={2}
          {...form.getInputProps('explanation')}
        />
        <Select
          required
          data={(metadata.data?.flashcard_retention_levels ?? []).map((level) => ({
            label: level.display_name ?? level.name,
            value: String(level.id),
          }))}
          disabled={metadata.isPending || metadata.isError}
          label="Erinnerungsstufe"
          {...form.getInputProps('retention_levels_id')}
        />
        <Switch
          label="Aktiviert"
          {...form.getInputProps('is_enabled', { type: 'checkbox' })}
        />
        {metadata.isError && (
          <Alert color="red" title="Fehler">
            Die Erinnerungsstufen konnten nicht geladen werden.
          </Alert>
        )}
        {(createFlashcard.isError || updateFlashcard.isError) && (
          <Alert color="red" title="Fehler">
            Die Lernkarte konnte nicht gespeichert werden.
          </Alert>
        )}
        <Group justify="flex-end">
          <Button
            disabled={isSaving}
            type="button"
            variant="default"
            onClick={onClose}
          >
            Abbrechen
          </Button>
          <Button loading={isSaving} type="submit">
            Speichern
          </Button>
        </Group>
      </Stack>
    </Box>
  )
}

function FlashcardDialog({
  document,
  flashcards,
  onClose,
}: {
  document: Document
  flashcards: Flashcard[]
  onClose: () => void
}) {
  const [editedFlashcard, setEditedFlashcard] = useState<
    Flashcard | null | undefined
  >(undefined)
  const [deletedFlashcard, setDeletedFlashcard] = useState<Flashcard | null>(null)
  const deleteFlashcard = useDeleteFlashcard()
  const documentFlashcards = flashcards.filter(
    (flashcard) => flashcard.document_id === document.entity_id,
  )
  const closeDelete = () => {
    deleteFlashcard.reset()
    setDeletedFlashcard(null)
  }

  return (
    <>
      <Modal
        centered
        opened
        size="lg"
        title={editedFlashcard === undefined ? 'Lernkarten' : (
          editedFlashcard === null ? 'Lernkarte erstellen' : 'Lernkarte bearbeiten'
        )}
        onClose={onClose}
      >
        {editedFlashcard !== undefined ? (
          <FlashcardForm
            documentId={document.entity_id}
            flashcard={editedFlashcard}
            onClose={() => setEditedFlashcard(undefined)}
          />
        ) : (
          <Stack gap="sm">
            <Group justify="space-between">
              <Text c="dimmed" size="sm">
                {documentFlashcards.length} Lernkarten
              </Text>
              <Button
                leftSection={<IconPlus size={16} />}
                size="compact-sm"
                onClick={() => setEditedFlashcard(null)}
              >
                Neu
              </Button>
            </Group>

            {documentFlashcards.length === 0 ? (
              <Text c="dimmed" py="lg" ta="center" size="sm">
                Für dieses Dokument gibt es noch keine Lernkarten.
              </Text>
            ) : (
              <Stack gap="xs">
                {documentFlashcards.map((flashcard) => (
                  <Paper key={flashcard.id} p="sm" withBorder>
                    <Group align="flex-start" justify="space-between" wrap="nowrap">
                      <SimpleGrid cols={2} spacing="sm" style={{ flex: 1 }}>
                        <Box>
                          <Text c="dimmed" size="xs">Vorderseite</Text>
                          <Text lineClamp={3} size="sm">{flashcard.front}</Text>
                        </Box>
                        <Box>
                          <Text c="dimmed" size="xs">Rückseite</Text>
                          <Text lineClamp={3} size="sm">{flashcard.back}</Text>
                        </Box>
                      </SimpleGrid>
                      <Group gap={4} wrap="nowrap">
                        <ActionIcon
                          aria-label="Lernkarte bearbeiten"
                          size="sm"
                          variant="subtle"
                          onClick={() => setEditedFlashcard(flashcard)}
                        >
                          <IconPencil size={16} />
                        </ActionIcon>
                        <ActionIcon
                          aria-label="Lernkarte löschen"
                          color="red"
                          size="sm"
                          variant="subtle"
                          onClick={() => setDeletedFlashcard(flashcard)}
                        >
                          <IconTrash size={16} />
                        </ActionIcon>
                      </Group>
                    </Group>
                  </Paper>
                ))}
              </Stack>
            )}
          </Stack>
        )}
      </Modal>

      {deletedFlashcard !== null && (
        <Modal centered opened title="Lernkarte löschen" onClose={closeDelete}>
          <Stack>
            <Text>Möchtest du diese Lernkarte wirklich löschen?</Text>
            <Text c="dimmed" lineClamp={2} size="sm">
              {deletedFlashcard.front}
            </Text>
            {deleteFlashcard.isError && (
              <Alert color="red" title="Fehler">
                Die Lernkarte konnte nicht gelöscht werden.
              </Alert>
            )}
            <Group justify="flex-end">
              <Button
                disabled={deleteFlashcard.isPending}
                type="button"
                variant="default"
                onClick={closeDelete}
              >
                Abbrechen
              </Button>
              <Button
                color="red"
                loading={deleteFlashcard.isPending}
                onClick={() => deleteFlashcard.mutate(
                  deletedFlashcard.id,
                  { onSuccess: closeDelete },
                )}
              >
                Löschen
              </Button>
            </Group>
          </Stack>
        </Modal>
      )}
    </>
  )
}

function LoadedDocumentPanel({
  topic,
  document,
  flashcards,
  drafts,
  onClose,
}: {
  topic: Topic
  document: Document
  flashcards: Flashcard[]
  drafts: Map<number, string>
  onClose: () => void
}) {
  const queryClient = useQueryClient()
  const [flashcardsOpened, setFlashcardsOpened] = useState(false)
  const [initialMarkdown] = useState(
    () => drafts.get(topic.entity_id) ?? document.content,
  )
  const [saveStatus, setSaveStatus] = useState<SaveStatus>(
    drafts.has(topic.entity_id) ? 'error' : 'saved',
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
        if (drafts.get(topic.entity_id) === content) {
          drafts.delete(topic.entity_id)
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

  const handleChange = useCallback((markdown: string) => {
    draftRef.current = markdown
    drafts.set(topic.entity_id, markdown)
    setSaveStatus('saving')
    if (timerRef.current !== null) clearTimeout(timerRef.current)
    timerRef.current = setTimeout(() => queueSave(markdown, true), 1_500)
  }, [drafts, queueSave, topic.entity_id])

  const statusLabel =
    saveStatus === 'saved'
      ? 'Gespeichert'
      : saveStatus === 'saving'
        ? 'Speichert …'
        : 'Fehler'

  return (
    <Paper
      className="realization-document-panel"
      h="100%"
      radius={0}
      w="100%"
      withBorder
      style={{ display: 'flex', flexDirection: 'column' }}
    >
      <Group justify="space-between" p="sm">
        <Group gap="xs">
          <Text fw={600}>{topic.name}</Text>
          <Button
            leftSection={<IconCards size={16} />}
            size="compact-sm"
            variant="subtle"
            onClick={() => setFlashcardsOpened(true)}
          >
            Lernkarten
          </Button>
        </Group>
        <Group gap="xs">
          <Text c={saveStatus === 'error' ? 'red' : 'dimmed'} size="sm">
            {statusLabel}
          </Text>
          <ActionIcon
            aria-label="Editor schließen"
            size="sm"
            variant="subtle"
            onClick={onClose}
          >
            <IconX size={16} />
          </ActionIcon>
        </Group>
      </Group>
      <Box style={{ flex: 1, minHeight: 0, overflow: 'auto' }}>
        <CrepeMarkdownEditor
          key={topic.entity_id}
          markdown={initialMarkdown}
          onChange={handleChange}
        />
      </Box>
      {flashcardsOpened && (
        <FlashcardDialog
          document={document}
          flashcards={flashcards}
          onClose={() => setFlashcardsOpened(false)}
        />
      )}
    </Paper>
  )
}

export function TopicDocumentPanel({
  topic,
  document,
  flashcards,
  drafts,
  onClose,
}: {
  topic: Topic
  document: Document | null
  flashcards: Flashcard[]
  drafts: Map<number, string>
  onClose: () => void
}) {
  const topicDocument = useTopicDocument(
    topic.entity_id,
    document?.entity_id ?? null,
  )

  if (topicDocument.isPending) {
    return (
      <Paper h="100%" radius={0} w="100%" withBorder>
        <Center h="100%"><Loader /></Center>
      </Paper>
    )
  }

  if (topicDocument.isError) {
    return (
      <Paper h="100%" p="md" radius={0} w="100%" withBorder>
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
      flashcards={flashcards}
      onClose={onClose}
      topic={topic}
    />
  )
}

export function TopicGraphView() {
  const topicGraph = useTopicGraph()
  const createTopic = useCreateTopic()
  const [nodes, setNodes, onNodesChange] = useNodesState<TopicNode>([])
  const [edges, setEdges, onEdgesChange] = useEdgesState<Edge>([])
  const [contextMenu, setContextMenu] = useState<ContextMenuPosition | null>(null)
  const [parentTopic, setParentTopic] = useState<TopicNode | null>(null)
  const [selectedTopicId, setSelectedTopicId] = useState<string | null>(null)
  const [createOpened, setCreateOpened] = useState(false)
  const flowRef = useRef<HTMLDivElement>(null)
  const flowInstanceRef = useRef<ReactFlowInstance<TopicNode, Edge> | null>(null)
  const layoutSignatureRef = useRef('')
  const [documentDrafts] = useState(() => new Map<number, string>())
  const form = useForm<TopicFormValues>({
    initialValues: { name: '' },
    validate: {
      name: (value) => value.trim() === '' ? 'Name ist erforderlich.' : null,
    },
  })
  const selectedTopic = nodes.find((node) => node.id === selectedTopicId)

  useEffect(() => {
    if (topicGraph.data === undefined) return

    const nextNodes: TopicNode[] = topicGraph.data.topics.map(({
      topic,
      document,
      flashcards,
    }) => ({
      id: String(topic.entity_id),
      data: { document, flashcards, label: topic.name, topic },
      position: { x: 0, y: 0 },
    }))
    const nodeIds = new Set(nextNodes.map((node) => node.id))
    const nextEdges: Edge[] = topicGraph.data.topics.flatMap(({ topic }) =>
      topic.parent_topic_entity_id === null ||
      !nodeIds.has(String(topic.parent_topic_entity_id))
        ? []
        : [{
            id: `${topic.parent_topic_entity_id}-${topic.entity_id}`,
            source: String(topic.parent_topic_entity_id),
            target: String(topic.entity_id),
          }],
    )

    layoutSignatureRef.current = ''
    setEdges(nextEdges)
    setNodes(nextNodes)
  }, [setEdges, setNodes, topicGraph.data])

  useEffect(() => {
    if (
      nodes.length === 0 ||
      nodes.some((node) =>
        node.measured?.height === undefined ||
        node.measured.width === undefined,
      )
    ) return

    const signature = JSON.stringify({
      edges: edges.map((edge) => [edge.id, edge.source, edge.target]),
      nodes: nodes.map((node) => [
        node.id,
        node.measured?.width,
        node.measured?.height,
      ]),
    })
    if (layoutSignatureRef.current === signature) return

    layoutSignatureRef.current = signature
    const controller = new AbortController()
    const flowBounds = flowRef.current?.getBoundingClientRect()
    void applyElkLayout(
      nodes,
      edges,
      flowBounds?.width ?? 1_000,
      flowBounds?.height ?? 700,
    ).then((layoutedNodes) => {
      if (controller.signal.aborted) return

      const positions = new Map(
        layoutedNodes.map((node) => [node.id, node.position]),
      )
      setNodes((currentNodes) => currentNodes.map((node) => ({
        ...node,
        position: positions.get(node.id) ?? node.position,
      })))
      requestAnimationFrame(() => {
        void flowInstanceRef.current?.fitView()
      })
    }).catch(() => {
      if (
        !controller.signal.aborted &&
        layoutSignatureRef.current === signature
      ) {
        layoutSignatureRef.current = ''
      }
    })

    return () => {
      controller.abort()
    }
  }, [edges, nodes, setNodes])

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
        development_area_id: parentTopic === null
          ? topicGraph.data?.development_areas[0]?.id ?? null
          : null,
        name,
      },
      {
        onSuccess: closeCreate,
      },
    )
  }

  return (
    <Box h="100%" pos="relative" w="100%">
      <PanelGroup
        key={selectedTopic === undefined ? 'graph' : 'split'}
        defaultLayout={
          selectedTopic === undefined
            ? { graph: 100 }
            : { editor: 35, graph: 65 }
        }
        orientation="horizontal"
        style={{ height: '100%', width: '100%' }}
      >
        <Panel id="graph" minSize="25%">
          <Box ref={flowRef} h="100%" pos="relative" w="100%">
            <ReactFlow
              fitView
              edges={edges}
              nodes={nodes}
              onEdgesChange={onEdgesChange}
              onNodeClick={(_, node) => setSelectedTopicId(node.id)}
              onNodeContextMenu={(event, node) => openContextMenu(event, node)}
              onNodesChange={onNodesChange}
              onInit={(instance) => {
                flowInstanceRef.current = instance
              }}
              onPaneClick={() => setContextMenu(null)}
              onPaneContextMenu={(event) => openContextMenu(event, null)}
            >
              <Background />
            </ReactFlow>

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
          </Box>
        </Panel>

        {selectedTopic !== undefined && (
          <>
            <PanelSeparator
              style={{
                background: 'var(--mantine-color-gray-3)',
                width: 6,
              }}
            />
            <Panel id="editor" minSize="25%">
              <TopicDocumentPanel
                key={selectedTopic.id}
                document={selectedTopic.data.document}
                drafts={documentDrafts}
                flashcards={selectedTopic.data.flashcards}
                topic={selectedTopic.data.topic}
                onClose={() => setSelectedTopicId(null)}
              />
            </Panel>
          </>
        )}
      </PanelGroup>

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
