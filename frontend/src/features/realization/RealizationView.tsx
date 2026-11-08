import { useState, type MouseEvent as ReactMouseEvent } from 'react'
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
  TextInput,
} from '@mantine/core'
import { useForm } from '@mantine/form'
import { useCreateTopic } from './api'

interface TopicNodeData extends Record<string, unknown> {
  label: string
}

type TopicNode = Node<TopicNodeData>

interface ContextMenuPosition {
  x: number
  y: number
}

interface TopicFormValues {
  name: string
}

function applyDagreLayout(nodes: TopicNode[], edges: Edge[]) {
  const graph = new dagre.graphlib.Graph()
  graph.setDefaultEdgeLabel(() => ({}))
  graph.setGraph({ rankdir: 'TB' })

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
  const createTopic = useCreateTopic()
  const [nodes, setNodes, onNodesChange] = useNodesState<TopicNode>([])
  const [edges, , onEdgesChange] = useEdgesState<Edge>([])
  const [contextMenu, setContextMenu] = useState<ContextMenuPosition | null>(null)
  const [createOpened, setCreateOpened] = useState(false)
  const form = useForm<TopicFormValues>({
    initialValues: { name: '' },
    validate: {
      name: (value) => value.trim() === '' ? 'Name ist erforderlich.' : null,
    },
  })

  const openContextMenu = (event: ReactMouseEvent) => {
    event.preventDefault()
    const bounds = event.currentTarget.getBoundingClientRect()
    setContextMenu({
      x: event.clientX - bounds.left,
      y: event.clientY - bounds.top,
    })
  }

  const openCreate = () => {
    setContextMenu(null)
    setCreateOpened(true)
  }

  const closeCreate = () => {
    createTopic.reset()
    form.reset()
    setCreateOpened(false)
  }

  const submit = (values: TopicFormValues) => {
    const name = values.name.trim()
    createTopic.mutate(
      {
        name,
      },
      {
        onSuccess: ({ entity_id }) => {
          setNodes((currentNodes) =>
            applyDagreLayout(
              [
                ...currentNodes,
                {
                  id: String(entity_id),
                  data: { label: name },
                  position: { x: 0, y: 0 },
                },
              ],
              edges,
            ),
          )
          closeCreate()
        },
      },
    )
  }

  return (
    <Box h="100%" pos="relative" w="100%" onContextMenu={openContextMenu}>
      <ReactFlow
        fitView
        edges={edges}
        nodes={nodes}
        onEdgesChange={onEdgesChange}
        onNodesChange={onNodesChange}
        onPaneClick={() => setContextMenu(null)}
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
