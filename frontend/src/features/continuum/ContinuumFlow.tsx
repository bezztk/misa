import { useMemo } from 'react'
import { Alert, Box, Center, Loader, Stack, Text, Title } from '@mantine/core'
import { graphlib, layout } from '@dagrejs/dagre'
import {
  Background,
  Controls,
  MarkerType,
  Position,
  ReactFlow,
} from '@xyflow/react'
import type { Edge, Node } from '@xyflow/react'
import '@xyflow/react/dist/style.css'
import { useTasks } from '../tasks/api'
import {
  useDevelopmentAreas,
  useDevelopmentCategories,
  useGoals,
  useItemHierarchy,
  useSkills,
} from './api'
import type {
  DevelopmentArea,
  DevelopmentCategory,
  Goal,
  ItemHierarchy,
  Skill,
} from './types'
import type { Task } from '../tasks/types'

const nodeWidth = 220
const nodeHeight = 56

function createFlowElements(
  areas: DevelopmentArea[],
  categories: DevelopmentCategory[],
  skills: Skill[],
  goals: Goal[],
  tasks: Task[],
  hierarchy: ItemHierarchy[],
) {
  const nodes: Node[] = []
  const edges: Edge[] = []
  const itemIds = new Set<number>()

  for (const area of areas) {
    nodes.push({
      id: `area-${area.id}`,
      data: { label: `Bereich: ${area.name}` },
      position: { x: 0, y: 0 },
      style: { width: nodeWidth },
    })
  }

  const addItem = (itemId: number, areaId: number, label: string) => {
    itemIds.add(itemId)
    nodes.push({
      id: `item-${itemId}`,
      data: { label },
      position: { x: 0, y: 0 },
      style: { width: nodeWidth },
    })
    edges.push({
      id: `area-${areaId}-item-${itemId}`,
      source: `area-${areaId}`,
      target: `item-${itemId}`,
      markerEnd: { type: MarkerType.ArrowClosed },
    })
  }

  for (const category of categories) {
    addItem(category.item_id, category.area_id, `Kategorie: ${category.name}`)
  }

  for (const skill of skills) {
    addItem(skill.item_id, skill.area_id, `Fähigkeit: ${skill.name}`)
  }

  for (const goal of goals) {
    addItem(goal.item_id, goal.area_id, `Ziel: ${goal.name}`)
  }

  for (const task of tasks) {
    if (task.area_id !== null) {
      addItem(task.item_id, task.area_id, `Aufgabe: ${task.name}`)
    }
  }

  for (const relation of hierarchy) {
    if (itemIds.has(relation.parent_id) && itemIds.has(relation.child_id)) {
      edges.push({
        id: `hierarchy-${relation.parent_id}-${relation.child_id}`,
        source: `item-${relation.parent_id}`,
        target: `item-${relation.child_id}`,
        markerEnd: { type: MarkerType.ArrowClosed },
      })
    }
  }

  const graph = new graphlib.Graph()
    .setDefaultEdgeLabel(() => ({}))
    .setGraph({ rankdir: 'TB', ranksep: 90, nodesep: 40 })

  for (const node of nodes) {
    graph.setNode(node.id, { width: nodeWidth, height: nodeHeight })
  }

  for (const edge of edges) {
    graph.setEdge(edge.source, edge.target)
  }

  layout(graph)

  return {
    nodes: nodes.map((node) => {
      const position = graph.node(node.id)

      return {
        ...node,
        position: {
          x: position.x - nodeWidth / 2,
          y: position.y - nodeHeight / 2,
        },
        sourcePosition: Position.Bottom,
        targetPosition: Position.Top,
      }
    }),
    edges,
  }
}

export function ContinuumFlow() {
  const areas = useDevelopmentAreas()
  const categories = useDevelopmentCategories()
  const skills = useSkills()
  const goals = useGoals()
  const tasks = useTasks()
  const hierarchy = useItemHierarchy()
  const isPending =
    areas.isPending ||
    categories.isPending ||
    skills.isPending ||
    goals.isPending ||
    tasks.isPending ||
    hierarchy.isPending
  const isError =
    areas.isError ||
    categories.isError ||
    skills.isError ||
    goals.isError ||
    tasks.isError ||
    hierarchy.isError
  const flow = useMemo(
    () =>
      createFlowElements(
        areas.data ?? [],
        categories.data ?? [],
        skills.data ?? [],
        goals.data ?? [],
        tasks.data ?? [],
        hierarchy.data ?? [],
      ),
    [
      areas.data,
      categories.data,
      goals.data,
      hierarchy.data,
      skills.data,
      tasks.data,
    ],
  )

  if (isPending) {
    return (
      <Stack>
        <Title>Continuum</Title>
        <Center>
          <Loader />
        </Center>
      </Stack>
    )
  }

  if (isError) {
    return (
      <Stack>
        <Title>Continuum</Title>
        <Alert color="red" title="Fehler">
          Die Continuum-Daten konnten nicht geladen werden.
        </Alert>
      </Stack>
    )
  }

  if (flow.nodes.length === 0) {
    return (
      <Stack>
        <Title>Continuum</Title>
        <Text>Keine Continuum-Daten vorhanden.</Text>
      </Stack>
    )
  }

  return (
    <Stack h="calc(100vh - 100px)">
      <Title>Continuum</Title>
      <Box style={{ flex: 1 }} mih={500}>
        <ReactFlow
          edges={flow.edges}
          elementsSelectable={false}
          fitView
          fitViewOptions={{ padding: 0.2 }}
          nodes={flow.nodes}
          nodesConnectable={false}
          nodesDraggable={false}
        >
          <Background />
          <Controls showInteractive={false} />
        </ReactFlow>
      </Box>
    </Stack>
  )
}
