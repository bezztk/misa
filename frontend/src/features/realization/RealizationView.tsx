import { useEffect, useRef, useState } from 'react'
import {
  Background,
  ReactFlow,
  useEdgesState,
  useNodesState,
  type Edge,
  type Node,
} from '@xyflow/react'
import '@xyflow/react/dist/style.css'
import dagre from '@dagrejs/dagre'
import {
  Alert,
  Box,
  Button,
  Group,
  Modal,
  Paper,
  Stack,
  Text,
  TextInput,
} from '@mantine/core'
import { useForm } from '@mantine/form'
import { useCreateTopic, useTopicGraph } from './api'
import type { Document, Topic } from './types'

interface TopicNodeData extends Record<string, unknown> {
  document: Document
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

export function RealizationView() {
  const topicGraph = useTopicGraph()
  const createTopic = useCreateTopic()
  const [nodes, setNodes, onNodesChange] = useNodesState<TopicNode>([])
  const [edges, setEdges, onEdgesChange] = useEdgesState<Edge>([])
  const [contextMenu, setContextMenu] = useState<ContextMenuPosition | null>(null)
  const [parentTopic, setParentTopic] = useState<TopicNode | null>(null)
  const [createOpened, setCreateOpened] = useState(false)
  const flowRef = useRef<HTMLDivElement>(null)
  const form = useForm<TopicFormValues>({
    initialValues: { name: '' },
    validate: {
      name: (value) => value.trim() === '' ? 'Name ist erforderlich.' : null,
    },
  })

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
        onNodeContextMenu={(event, node) => openContextMenu(event, node)}
        onNodesChange={onNodesChange}
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
