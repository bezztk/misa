import { useMemo, useState } from 'react'
import {
  Alert,
  Box,
  Button,
  Center,
  Checkbox,
  Group,
  Loader,
  Modal,
  Select,
  Stack,
  Text,
  Textarea,
  TextInput,
  Title,
} from '@mantine/core'
import { useForm } from '@mantine/form'
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
import { useTaskMetadata, useTasks, useUpdateTask } from '../tasks/api'
import {
  useDevelopmentAreas,
  useDevelopmentCategories,
  useGoals,
  useItemHierarchy,
  useSkillMetadata,
  useSkills,
  useUpdateDevelopmentArea,
  useUpdateDevelopmentCategory,
  useUpdateGoal,
  useUpdateSkill,
} from './api'
import type {
  DevelopmentArea,
  DevelopmentCategory,
  Goal,
  ItemHierarchy,
  Skill,
  UpdateDevelopmentArea,
  UpdateDevelopmentCategory,
  UpdateGoal,
  UpdateSkill,
} from './types'
import type { Task, UpdateTask } from '../tasks/types'

const nodeWidth = 220
const nodeHeight = 56

type SelectedEntity =
  | { type: 'area'; value: DevelopmentArea }
  | { type: 'category'; value: DevelopmentCategory }
  | { type: 'skill'; value: Skill }
  | { type: 'goal'; value: Goal }
  | { type: 'task'; value: Task }

interface ContinuumNodeData extends Record<string, unknown> {
  label: string
  entity: SelectedEntity
}

type ContinuumNode = Node<ContinuumNodeData>

function createFlowElements(
  areas: DevelopmentArea[],
  categories: DevelopmentCategory[],
  skills: Skill[],
  goals: Goal[],
  tasks: Task[],
  hierarchy: ItemHierarchy[],
) {
  const nodes: ContinuumNode[] = []
  const edges: Edge[] = []
  const layoutEdges: Array<{ source: string; target: string }> = []
  const areaIds = new Set<number>()
  const itemIds = new Set<number>()
  const itemAreaIds = new Map<number, number>()

  for (const area of areas) {
    areaIds.add(area.id)
    nodes.push({
      id: `area-${area.id}`,
      data: {
        label: `Bereich: ${area.name}`,
        entity: { type: 'area', value: area },
      },
      position: { x: 0, y: 0 },
      style: { cursor: 'pointer', width: nodeWidth },
    })
  }

  const addItem = (
    itemId: number,
    areaId: number,
    label: string,
    entity: SelectedEntity,
  ) => {
    itemIds.add(itemId)
    itemAreaIds.set(itemId, areaId)
    nodes.push({
      id: `item-${itemId}`,
      data: { label, entity },
      position: { x: 0, y: 0 },
      style: { cursor: 'pointer', width: nodeWidth },
    })
  }

  for (const category of categories) {
    addItem(
      category.item_id,
      category.area_id,
      `Kategorie: ${category.name}`,
      { type: 'category', value: category },
    )
  }

  for (const skill of skills) {
    addItem(skill.item_id, skill.area_id, `Fähigkeit: ${skill.name}`, {
      type: 'skill',
      value: skill,
    })
  }

  for (const goal of goals) {
    addItem(goal.item_id, goal.area_id, `Ziel: ${goal.name}`, {
      type: 'goal',
      value: goal,
    })
  }

  for (const task of tasks) {
    if (task.area_id !== null) {
      addItem(task.item_id, task.area_id, `Aufgabe: ${task.name}`, {
        type: 'task',
        value: task,
      })
    }
  }

  const validHierarchy = hierarchy
    .filter(
      (relation) =>
        itemIds.has(relation.parent_id) && itemIds.has(relation.child_id),
    )
    .sort(
      (left, right) =>
        left.child_id - right.child_id || left.parent_id - right.parent_id,
    )
  const structuralParents = new Map<number, number>()

  for (const relation of validHierarchy) {
    if (!structuralParents.has(relation.child_id)) {
      structuralParents.set(relation.child_id, relation.parent_id)
    }
  }

  for (const [itemId, areaId] of itemAreaIds) {
    if (!areaIds.has(areaId)) continue

    const edge = {
      id: `area-${areaId}-item-${itemId}`,
      source: `area-${areaId}`,
      target: `item-${itemId}`,
      markerEnd: { type: MarkerType.ArrowClosed },
    }
    const isStructural = !structuralParents.has(itemId)

    edges.push(
      isStructural ? edge : { ...edge, style: { strokeDasharray: '5 5' } },
    )

    if (isStructural) {
      layoutEdges.push({ source: edge.source, target: edge.target })
    }
  }

  for (const relation of validHierarchy) {
    const edge = {
      id: `hierarchy-${relation.parent_id}-${relation.child_id}`,
      source: `item-${relation.parent_id}`,
      target: `item-${relation.child_id}`,
      markerEnd: { type: MarkerType.ArrowClosed },
    }
    const isStructural =
      structuralParents.get(relation.child_id) === relation.parent_id

    edges.push(
      isStructural ? edge : { ...edge, style: { strokeDasharray: '5 5' } },
    )

    if (isStructural) {
      layoutEdges.push({ source: edge.source, target: edge.target })
    }
  }

  const graph = new graphlib.Graph()
    .setDefaultEdgeLabel(() => ({}))
    .setGraph({ rankdir: 'TB', ranksep: 90, nodesep: 40 })

  for (const node of nodes) {
    graph.setNode(node.id, { width: nodeWidth, height: nodeHeight })
  }

  for (const edge of layoutEdges) {
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

interface EditFormValues {
  name: string
  description: string
  state_id: string | null
  is_completed: boolean
  due_at: string
}

function toLocalDateTime(value: string | null) {
  if (value === null) return ''

  const date = new Date(value)
  const localDate = new Date(date.getTime() - date.getTimezoneOffset() * 60_000)
  return localDate.toISOString().slice(0, 16)
}

function ContinuumEditModal({
  entity,
  areaName,
  onClose,
}: {
  entity: SelectedEntity
  areaName: string | null
  onClose: () => void
}) {
  const updateArea = useUpdateDevelopmentArea()
  const updateCategory = useUpdateDevelopmentCategory()
  const updateSkill = useUpdateSkill()
  const updateGoal = useUpdateGoal()
  const updateTask = useUpdateTask()
  const skillMetadata = useSkillMetadata(entity.type === 'skill')
  const taskMetadata = useTaskMetadata(entity.type === 'task')
  const description =
    entity.type === 'category' ? '' : (entity.value.description ?? '')
  const form = useForm<EditFormValues>({
    initialValues: {
      name: entity.value.name,
      description,
      state_id:
        entity.type === 'skill' || entity.type === 'task'
          ? String(entity.value.state_id)
          : null,
      is_completed: entity.type === 'goal' && entity.value.is_completed,
      due_at:
        entity.type === 'goal' ? toLocalDateTime(entity.value.due_at) : '',
    },
    validate: {
      name: (value) => {
        const length = [...value.trim()].length
        const maximum = entity.type === 'task' ? 150 : 50

        if (length === 0) return 'Name ist erforderlich.'
        if (length > maximum) {
          return `Der Name darf maximal ${maximum} Zeichen haben.`
        }
        return null
      },
      state_id: (value) =>
        (entity.type === 'skill' || entity.type === 'task') && value === null
          ? 'Status ist erforderlich.'
          : null,
    },
  })
  const isMetadataPending =
    (entity.type === 'skill' && skillMetadata.isPending) ||
    (entity.type === 'task' && taskMetadata.isPending)
  const isMetadataError =
    (entity.type === 'skill' && skillMetadata.isError) ||
    (entity.type === 'task' && taskMetadata.isError)
  const isSaving =
    updateArea.isPending ||
    updateCategory.isPending ||
    updateSkill.isPending ||
    updateGoal.isPending ||
    updateTask.isPending
  const isError =
    updateArea.isError ||
    updateCategory.isError ||
    updateSkill.isError ||
    updateGoal.isError ||
    updateTask.isError

  const submit = (values: EditFormValues) => {
    const name = values.name.trim()
    const description = values.description.trim()

    if (entity.type === 'area') {
      const area: UpdateDevelopmentArea = { name, description }
      updateArea.mutate(
        { id: entity.value.id, area },
        { onSuccess: onClose },
      )
      return
    }

    if (entity.type === 'category') {
      const category: UpdateDevelopmentCategory = { name }
      updateCategory.mutate(
        { id: entity.value.item_id, category },
        { onSuccess: onClose },
      )
      return
    }

    if (entity.type === 'skill') {
      const skill: UpdateSkill = {
        name,
        description,
        state_id:
          values.state_id === null
            ? entity.value.state_id
            : Number(values.state_id),
      }
      updateSkill.mutate(
        { id: entity.value.item_id, skill },
        { onSuccess: onClose },
      )
      return
    }

    if (entity.type === 'goal') {
      const goal: UpdateGoal = {
        name,
        description,
        is_completed: values.is_completed,
        due_at: values.due_at
          ? new Date(values.due_at).toISOString()
          : undefined,
        clear_due_at: values.due_at === '' && entity.value.due_at !== null,
      }
      updateGoal.mutate(
        { id: entity.value.item_id, goal },
        { onSuccess: onClose },
      )
      return
    }

    const task: UpdateTask = {
      name,
      description,
      state_id:
        values.state_id === null
          ? entity.value.state_id
          : Number(values.state_id),
    }
    updateTask.mutate(
      { id: entity.value.item_id, task },
      { onSuccess: onClose },
    )
  }

  return (
    <Modal centered opened onClose={onClose} title="Element bearbeiten">
      {isMetadataPending ? (
        <Center>
          <Loader />
        </Center>
      ) : isMetadataError ? (
        <Alert color="red" title="Fehler">
          Die Auswahldaten konnten nicht geladen werden.
        </Alert>
      ) : (
        <Box component="form" onSubmit={form.onSubmit(submit)}>
          <Stack>
            <TextInput
              data-autofocus
              required
              label="Name"
              {...form.getInputProps('name')}
            />
            {areaName !== null && (
              <TextInput disabled label="Continuum-Bereich" value={areaName} />
            )}
            {entity.type !== 'category' && (
              <Textarea
                autosize
                label="Beschreibung"
                minRows={3}
                {...form.getInputProps('description')}
              />
            )}
            {entity.type === 'skill' && (
              <Select
                required
                label="Status"
                data={(skillMetadata.data?.states ?? []).map((state) => ({
                  value: String(state.id),
                  label: state.name,
                }))}
                {...form.getInputProps('state_id')}
              />
            )}
            {entity.type === 'task' && (
              <Select
                required
                label="Status"
                data={(taskMetadata.data?.states ?? []).map((state) => ({
                  value: String(state.id),
                  label: state.name,
                }))}
                {...form.getInputProps('state_id')}
              />
            )}
            {entity.type === 'goal' && (
              <>
                <Checkbox
                  label="Abgeschlossen"
                  {...form.getInputProps('is_completed', {
                    type: 'checkbox',
                  })}
                />
                <TextInput
                  label="Fällig am"
                  type="datetime-local"
                  {...form.getInputProps('due_at')}
                />
              </>
            )}
            {isError && (
              <Alert color="red" title="Fehler">
                Das Element konnte nicht gespeichert werden.
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
      )}
    </Modal>
  )
}

export function ContinuumFlow() {
  const [selectedEntity, setSelectedEntity] = useState<SelectedEntity | null>(
    null,
  )
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
  const selectedAreaName =
    selectedEntity === null || selectedEntity.type === 'area'
      ? null
      : (areas.data?.find(
          (area) => area.id === selectedEntity.value.area_id,
        )?.name ?? '–')

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
          onNodeClick={(_, node) => setSelectedEntity(node.data.entity)}
        >
          <Background />
          <Controls showInteractive={false} />
        </ReactFlow>
      </Box>
      {selectedEntity !== null && (
        <ContinuumEditModal
          areaName={selectedAreaName}
          entity={selectedEntity}
          onClose={() => setSelectedEntity(null)}
        />
      )}
    </Stack>
  )
}
