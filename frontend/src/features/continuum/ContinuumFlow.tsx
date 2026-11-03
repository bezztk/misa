import { useMemo, useState } from 'react'
import {
  Alert,
  Badge,
  Box,
  Button,
  Center,
  Checkbox,
  Group,
  Loader,
  Modal,
  Paper,
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
  Handle,
  MarkerType,
  Position,
  ReactFlow,
  ViewportPortal,
} from '@xyflow/react'
import type { Edge, Node, NodeProps, NodeTypes } from '@xyflow/react'
import '@xyflow/react/dist/style.css'
import {
  useCreateTask,
  useDeleteTask,
  useTaskMetadata,
  useTasks,
  useUpdateTask,
} from '../tasks/api'
import {
  useAllowedTypeRelations,
  useCreateDevelopmentArea,
  useCreateDevelopmentCategory,
  useCreateGoal,
  useCreateSkill,
  useDeleteDevelopmentArea,
  useDeleteDevelopmentCategory,
  useDeleteGoal,
  useDeleteSkill,
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
  useUpdateItemParent,
} from './api'
import type {
  CreateDevelopmentArea,
  CreateDevelopmentCategory,
  CreateGoal,
  CreateSkill,
  DevelopmentArea,
  DevelopmentCategory,
  Goal,
  ItemHierarchy,
  Skill,
  SkillState,
  UpdateDevelopmentArea,
  UpdateDevelopmentCategory,
  UpdateGoal,
  UpdateSkill,
} from './types'
import type {
  CreateTask,
  Task,
  TaskState,
  UpdateTask,
} from '../tasks/types'

const areaGap = 280

type SelectedEntity =
  | { type: 'area'; value: DevelopmentArea }
  | { type: 'category'; value: DevelopmentCategory }
  | { type: 'skill'; value: Skill }
  | { type: 'goal'; value: Goal }
  | { type: 'task'; value: Task }

interface ContinuumNodeData extends Record<string, unknown> {
  label: string
  entity: SelectedEntity
  statusColor?: string
  statusLabel?: string
}

type ContinuumNodeType = SelectedEntity['type']
type ContinuumNode = Node<ContinuumNodeData, ContinuumNodeType>

const nodeWidths: Record<
  ContinuumNodeType,
  { min: number; max: number }
> = {
  area: { min: 280, max: 340 },
  skill: { min: 280, max: 340 },
  goal: { min: 260, max: 320 },
  category: { min: 200, max: 260 },
  task: { min: 240, max: 320 },
}

const typeLabels: Record<ContinuumNodeType, string> = {
  area: 'Development Area',
  skill: 'Fähigkeit',
  goal: 'Ziel',
  category: 'Kategorie',
  task: 'Aufgabe',
}

function getNodeSize(data: ContinuumNodeData) {
  const type = data.entity.type
  const widths = nodeWidths[type]
  const width = Math.min(widths.max, widths.min + data.label.length * 3)
  const horizontalPadding = type === 'category' ? 16 : 28
  const charactersPerLine = Math.max(
    1,
    Math.floor((width - horizontalPadding) / 9),
  )
  const lines = Math.max(1, Math.ceil(data.label.length / charactersPerLine))
  const statusHeight =
    type !== 'task' && data.statusLabel !== undefined ? 22 : 0
  const verticalPadding = type === 'category' ? 14 : 24
  const height = verticalPadding + 18 + lines * 20 + statusHeight

  return { width, height }
}

function ContinuumNodeView({ data }: NodeProps<ContinuumNode>) {
  const type = data.entity.type
  const isArea = type === 'area'
  const isSkill = type === 'skill'
  const isGoal = type === 'goal'
  const isCategory = type === 'category'
  const size = getNodeSize(data)
  const borderColor = isSkill
    ? 'var(--mantine-color-blue-6)'
    : isGoal
      ? 'var(--mantine-color-violet-6)'
      : isArea
        ? 'var(--mantine-color-gray-7)'
        : 'var(--mantine-color-gray-4)'
  const labelColor = isSkill ? 'blue' : isGoal ? 'violet' : 'dimmed'
  const handleStyle = {
    width: 8,
    height: 8,
    border: '2px solid var(--mantine-color-body)',
    background: borderColor,
  }

  return (
    <>
      {!isArea && (
        <Handle position={Position.Top} style={handleStyle} type="target" />
      )}
      <Paper
        h={size.height}
        p={isCategory ? 7 : 'sm'}
        radius={isSkill ? 'md' : isCategory ? 'xs' : 'sm'}
        shadow={isSkill ? 'sm' : undefined}
        w={size.width}
        style={{
          border: `${isArea || isSkill ? 2 : 1}px solid ${borderColor}`,
          boxSizing: 'border-box',
          cursor: 'pointer',
        }}
      >
        <Stack gap={4} h="100%" justify="center">
          <Group gap={6} wrap="nowrap">
            {type === 'task' && data.statusLabel !== undefined && (
              <Box
                aria-label={`Status: ${data.statusLabel}`}
                title={data.statusLabel}
                style={{
                  flex: '0 0 auto',
                  width: 8,
                  height: 8,
                  borderRadius: '50%',
                  background: `var(--mantine-color-${data.statusColor ?? 'gray'}-6)`,
                }}
              />
            )}
            <Text c={labelColor} fw={700} size="10px" tt="uppercase">
              {typeLabels[type]}
            </Text>
          </Group>
          <Text
            fw={isSkill || isArea ? 700 : 500}
            lh={1.35}
            size={isCategory ? 'xs' : 'sm'}
            style={{ overflowWrap: 'anywhere' }}
          >
            {data.label}
          </Text>
          {type !== 'task' && data.statusLabel !== undefined && (
            <Badge color={data.statusColor ?? 'gray'} size="xs" variant="light">
              {data.statusLabel}
            </Badge>
          )}
        </Stack>
      </Paper>
      <Handle position={Position.Bottom} style={handleStyle} type="source" />
    </>
  )
}

const nodeTypes = {
  area: ContinuumNodeView,
  category: ContinuumNodeView,
  goal: ContinuumNodeView,
  skill: ContinuumNodeView,
  task: ContinuumNodeView,
} satisfies NodeTypes

function createFlowElements(
  areas: DevelopmentArea[],
  categories: DevelopmentCategory[],
  skills: Skill[],
  goals: Goal[],
  tasks: Task[],
  hierarchy: ItemHierarchy[],
  skillStates: SkillState[],
  taskStates: TaskState[],
) {
  const nodes: ContinuumNode[] = []
  const edges: Edge[] = []
  const layoutEdges: Array<{ source: string; target: string }> = []
  const areaIds = new Set<number>()
  const itemIds = new Set<number>()
  const itemAreaIds = new Map<number, number>()
  const skillStateNames = new Map(
    skillStates.map((state) => [state.id, state.name]),
  )
  const taskStateNames = new Map(
    taskStates.map((state) => [state.id, state.name]),
  )
  const taskStatusColors: Record<string, string> = {
    open: 'gray',
    in_progress: 'blue',
    completed: 'green',
    cancelled: 'red',
  }

  for (const area of areas) {
    areaIds.add(area.id)
    nodes.push({
      id: `area-${area.id}`,
      data: {
        label: area.name,
        entity: { type: 'area', value: area },
      },
      position: { x: 0, y: 0 },
      type: 'area',
    })
  }

  const addItem = (
    itemId: number,
    areaId: number,
    label: string,
    entity: SelectedEntity,
    statusLabel?: string,
    statusColor?: string,
  ) => {
    itemIds.add(itemId)
    itemAreaIds.set(itemId, areaId)
    nodes.push({
      id: `item-${itemId}`,
      data: { label, entity, statusColor, statusLabel },
      position: { x: 0, y: 0 },
      type: entity.type,
    })
  }

  for (const category of categories) {
    addItem(
      category.item_id,
      category.area_id,
      category.name,
      { type: 'category', value: category },
    )
  }

  for (const skill of skills) {
    addItem(
      skill.item_id,
      skill.area_id,
      skill.name,
      { type: 'skill', value: skill },
      skillStateNames.get(skill.state_id),
      'blue',
    )
  }

  for (const goal of goals) {
    addItem(
      goal.item_id,
      goal.area_id,
      goal.name,
      { type: 'goal', value: goal },
      goal.is_completed ? 'Erledigt' : 'Offen',
      goal.is_completed ? 'green' : 'gray',
    )
  }

  for (const task of tasks) {
    if (task.area_id !== null) {
      const statusLabel = taskStateNames.get(task.state_id)
      addItem(
        task.item_id,
        task.area_id,
        task.name,
        { type: 'task', value: task },
        statusLabel,
        statusLabel === undefined ? undefined : taskStatusColors[statusLabel],
      )
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

  const resolvedAreaIds = new Map<number, number>()

  const resolveAreaId = (
    itemId: number,
    visited = new Set<number>(),
  ): number | undefined => {
    const cachedAreaId = resolvedAreaIds.get(itemId)
    if (cachedAreaId !== undefined) return cachedAreaId

    const ownAreaId = itemAreaIds.get(itemId)
    const parentId = structuralParents.get(itemId)

    if (parentId === undefined || visited.has(itemId)) return ownAreaId

    visited.add(itemId)
    const parentAreaId: number | undefined = resolveAreaId(parentId, visited)
    visited.delete(itemId)

    const areaId: number | undefined = parentAreaId ?? ownAreaId
    if (areaId !== undefined) resolvedAreaIds.set(itemId, areaId)
    return areaId
  }

  const nodesById = new Map(nodes.map((node) => [node.id, node]))
  const groupedNodeIds = new Set<string>()
  const layoutGroups = areas.map((area) => {
    const nodeIds = new Set([
      `area-${area.id}`,
      ...[...itemIds]
        .filter((itemId) => resolveAreaId(itemId) === area.id)
        .map((itemId) => `item-${itemId}`),
    ])

    for (const nodeId of nodeIds) groupedNodeIds.add(nodeId)
    return nodeIds
  })
  const unassignedNodeIds = new Set(
    nodes
      .map((node) => node.id)
      .filter((nodeId) => !groupedNodeIds.has(nodeId)),
  )

  if (unassignedNodeIds.size > 0) layoutGroups.push(unassignedNodeIds)

  const areaLayouts = layoutGroups.map((nodeIds) => {
    const graph = new graphlib.Graph()
      .setDefaultEdgeLabel(() => ({}))
      .setGraph({ rankdir: 'TB', ranksep: 90, nodesep: 40 })

    for (const nodeId of nodeIds) {
      const node = nodesById.get(nodeId)
      if (node !== undefined) graph.setNode(nodeId, getNodeSize(node.data))
    }

    for (const edge of layoutEdges) {
      if (nodeIds.has(edge.source) && nodeIds.has(edge.target)) {
        graph.setEdge(edge.source, edge.target)
      }
    }

    layout(graph)

    const positions = new Map<string, { x: number; y: number }>()
    let minX = Number.POSITIVE_INFINITY
    let minY = Number.POSITIVE_INFINITY
    let maxX = Number.NEGATIVE_INFINITY
    let maxY = Number.NEGATIVE_INFINITY

    for (const nodeId of nodeIds) {
      const node = nodesById.get(nodeId)
      if (node === undefined) continue

      const size = getNodeSize(node.data)
      const position = graph.node(nodeId)
      const x = position.x - size.width / 2
      const y = position.y - size.height / 2
      positions.set(nodeId, { x, y })
      minX = Math.min(minX, x)
      minY = Math.min(minY, y)
      maxX = Math.max(maxX, x + size.width)
      maxY = Math.max(maxY, y + size.height)
    }

    for (const [nodeId, position] of positions) {
      positions.set(nodeId, {
        x: position.x - minX,
        y: position.y - minY,
      })
    }

    return {
      positions,
      width: maxX - minX,
      height: maxY - minY,
    }
  })

  const positions = new Map<string, { x: number; y: number }>()
  const separators: Array<{ x: number; y: number; height: number }> = []
  const separatorHeight = Math.max(
    72,
    ...areaLayouts.map((areaLayout) => areaLayout.height),
  )
  let offsetX = 0

  areaLayouts.forEach((areaLayout, index) => {
    for (const [nodeId, position] of areaLayout.positions) {
      positions.set(nodeId, { x: position.x + offsetX, y: position.y })
    }

    if (index < areaLayouts.length - 1) {
      separators.push({
        x: offsetX + areaLayout.width + areaGap / 2,
        y: -40,
        height: separatorHeight + 80,
      })
    }

    offsetX += areaLayout.width + areaGap
  })

  return {
    nodes: nodes.map((node) => {
      const position = positions.get(node.id) ?? { x: 0, y: 0 }

      return {
        ...node,
        position,
        sourcePosition: Position.Bottom,
        targetPosition: Position.Top,
      }
    }),
    edges,
    separators,
  }
}

interface EditFormValues {
  name: string
  description: string
  state_id: string | null
  is_completed: boolean
  due_at: string
  parent_id: string | null
}

function toLocalDateTime(value: string | null) {
  if (value === null) return ''

  const date = new Date(value)
  const localDate = new Date(date.getTime() - date.getTimezoneOffset() * 60_000)
  return localDate.toISOString().slice(0, 16)
}

type CreateEntityType = SelectedEntity['type']

const backendItemTypes: Record<Exclude<CreateEntityType, 'area'>, string> = {
  category: 'Category',
  skill: 'Skill',
  goal: 'Goal',
  task: 'Task',
}

function getParentCandidates(
  categories: DevelopmentCategory[],
  skills: Skill[],
  goals: Goal[],
  tasks: Task[],
) {
  return [
    ...categories.map((category) => ({
      areaId: category.area_id,
      id: category.item_id,
      label: `Kategorie: ${category.name}`,
      type: 'Category',
    })),
    ...skills.map((skill) => ({
      areaId: skill.area_id,
      id: skill.item_id,
      label: `Fähigkeit: ${skill.name}`,
      type: 'Skill',
    })),
    ...goals.map((goal) => ({
      areaId: goal.area_id,
      id: goal.item_id,
      label: `Ziel: ${goal.name}`,
      type: 'Goal',
    })),
    ...tasks
      .filter((task) => task.area_id !== null && task.group_id === null)
      .map((task) => ({
        areaId: task.area_id as number,
        id: task.item_id,
        label: `Aufgabe: ${task.name}`,
        type: 'Task',
      })),
  ]
}

function createsCycle(
  parentId: number,
  childId: number,
  hierarchy: ItemHierarchy[],
) {
  const parents = new Map(
    hierarchy.map((relation) => [relation.child_id, relation.parent_id]),
  )
  const visited = new Set<number>()
  let currentId: number | undefined = parentId

  while (currentId !== undefined && !visited.has(currentId)) {
    if (currentId === childId) return true
    visited.add(currentId)
    currentId = parents.get(currentId)
  }

  return false
}

function ContinuumEditModal({
  entity,
  areaName,
  categories,
  skills,
  goals,
  tasks,
  hierarchy,
  onClose,
  onDelete,
}: {
  entity: SelectedEntity
  areaName: string | null
  categories: DevelopmentCategory[]
  skills: Skill[]
  goals: Goal[]
  tasks: Task[]
  hierarchy: ItemHierarchy[]
  onClose: () => void
  onDelete: (entity: SelectedEntity) => void
}) {
  const updateArea = useUpdateDevelopmentArea()
  const updateCategory = useUpdateDevelopmentCategory()
  const updateSkill = useUpdateSkill()
  const updateGoal = useUpdateGoal()
  const updateTask = useUpdateTask()
  const updateParent = useUpdateItemParent()
  const skillMetadata = useSkillMetadata(entity.type === 'skill')
  const taskMetadata = useTaskMetadata(entity.type === 'task')
  const allowedRelations = useAllowedTypeRelations(entity.type !== 'area')
  const itemId = entity.type === 'area' ? null : entity.value.item_id
  const currentParentId =
    itemId === null
      ? null
      : (hierarchy.find((relation) => relation.child_id === itemId)
          ?.parent_id ?? null)
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
      parent_id:
        currentParentId === null ? null : String(currentParentId),
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
    (entity.type !== 'area' && allowedRelations.isPending) ||
    (entity.type === 'skill' && skillMetadata.isPending) ||
    (entity.type === 'task' && taskMetadata.isPending)
  const isMetadataError =
    (entity.type !== 'area' && allowedRelations.isError) ||
    (entity.type === 'skill' && skillMetadata.isError) ||
    (entity.type === 'task' && taskMetadata.isError)
  const isSaving =
    updateArea.isPending ||
    updateCategory.isPending ||
    updateSkill.isPending ||
    updateGoal.isPending ||
    updateTask.isPending ||
    updateParent.isPending
  const isError =
    updateArea.isError ||
    updateCategory.isError ||
    updateSkill.isError ||
    updateGoal.isError ||
    updateTask.isError ||
    updateParent.isError
  const entityAreaId =
    entity.type === 'area' ? null : entity.value.area_id
  const childType =
    entity.type === 'area' ? null : backendItemTypes[entity.type]
  const allowedParentTypes = new Set(
    (allowedRelations.data ?? [])
      .filter((relation) => relation.child_type === childType)
      .map((relation) => relation.parent_type),
  )
  const canHaveParent =
    entity.type !== 'task' || entity.value.group_id === null
  const parentOptions =
    itemId === null || !canHaveParent
      ? []
      : getParentCandidates(categories, skills, goals, tasks)
          .filter(
            (candidate) =>
              candidate.id !== itemId &&
              candidate.areaId === entityAreaId &&
              allowedParentTypes.has(candidate.type) &&
              !createsCycle(candidate.id, itemId, hierarchy),
          )
          .map((candidate) => ({
            value: String(candidate.id),
            label: candidate.label,
          }))

  const submit = async (values: EditFormValues) => {
    const name = values.name.trim()
    const description = values.description.trim()

    try {
      if (entity.type === 'area') {
        const area: UpdateDevelopmentArea = { name, description }
        await updateArea.mutateAsync({ id: entity.value.id, area })
      } else if (entity.type === 'category') {
        const category: UpdateDevelopmentCategory = { name }
        await updateCategory.mutateAsync({
          id: entity.value.item_id,
          category,
        })
      } else if (entity.type === 'skill') {
        const skill: UpdateSkill = {
          name,
          description,
          state_id:
            values.state_id === null
              ? entity.value.state_id
              : Number(values.state_id),
        }
        await updateSkill.mutateAsync({ id: entity.value.item_id, skill })
      } else if (entity.type === 'goal') {
        const goal: UpdateGoal = {
          name,
          description,
          is_completed: values.is_completed,
          due_at: values.due_at
            ? new Date(values.due_at).toISOString()
            : undefined,
          clear_due_at: values.due_at === '' && entity.value.due_at !== null,
        }
        await updateGoal.mutateAsync({ id: entity.value.item_id, goal })
      } else {
        const task: UpdateTask = {
          name,
          description,
          state_id:
            values.state_id === null
              ? entity.value.state_id
              : Number(values.state_id),
        }
        await updateTask.mutateAsync({ id: entity.value.item_id, task })
      }

      const nextParentId =
        values.parent_id === null ? null : Number(values.parent_id)

      if (itemId !== null && nextParentId !== currentParentId) {
        await updateParent.mutateAsync({
          id: itemId,
          item: { parent_id: nextParentId },
        })
      }

      onClose()
    } catch {
      return
    }
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
            {entity.type !== 'area' && (
              <Select
                clearable
                label="Übergeordnetes Element"
                data={parentOptions}
                {...form.getInputProps('parent_id')}
              />
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
            <Group justify="space-between">
              <Button
                color="red"
                disabled={isSaving}
                type="button"
                variant="light"
                onClick={() => onDelete(entity)}
              >
                Löschen
              </Button>
              <Group>
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
            </Group>
          </Stack>
        </Box>
      )}
    </Modal>
  )
}

interface CreateFormValues {
  type: CreateEntityType
  name: string
  description: string
  area_id: string | null
  state_id: string | null
  due_at: string
  parent_id: string | null
}

function CreateContinuumModal({
  areas,
  categories,
  skills,
  goals,
  tasks,
  onClose,
}: {
  areas: DevelopmentArea[]
  categories: DevelopmentCategory[]
  skills: Skill[]
  goals: Goal[]
  tasks: Task[]
  onClose: () => void
}) {
  const createArea = useCreateDevelopmentArea()
  const createCategory = useCreateDevelopmentCategory()
  const createSkill = useCreateSkill()
  const createGoal = useCreateGoal()
  const createTask = useCreateTask()
  const updateParent = useUpdateItemParent()
  const skillMetadata = useSkillMetadata(true)
  const allowedRelations = useAllowedTypeRelations(true)
  const form = useForm<CreateFormValues>({
    initialValues: {
      type: 'area',
      name: '',
      description: '',
      area_id: null,
      state_id: null,
      due_at: '',
      parent_id: null,
    },
    validate: {
      name: (value, values) => {
        const length = [...value.trim()].length
        const maximum = values.type === 'task' ? 150 : 50

        if (length === 0) return 'Name ist erforderlich.'
        if (length > maximum) {
          return `Der Name darf maximal ${maximum} Zeichen haben.`
        }
        return null
      },
      area_id: (value, values) =>
        values.type !== 'area' && value === null
          ? 'Continuum-Bereich ist erforderlich.'
          : null,
    },
  })
  const parentCandidates = getParentCandidates(
    categories,
    skills,
    goals,
    tasks,
  )
  const childType =
    form.values.type === 'area' ? null : backendItemTypes[form.values.type]
  const allowedParentTypes = new Set(
    (allowedRelations.data ?? [])
      .filter((relation) => relation.child_type === childType)
      .map((relation) => relation.parent_type),
  )
  const areaId =
    form.values.area_id === null ? null : Number(form.values.area_id)
  const parentOptions = parentCandidates
    .filter(
      (candidate) =>
        candidate.areaId === areaId && allowedParentTypes.has(candidate.type),
    )
    .map((candidate) => ({
      value: String(candidate.id),
      label: candidate.label,
    }))
  const isPending =
    createArea.isPending ||
    createCategory.isPending ||
    createSkill.isPending ||
    createGoal.isPending ||
    createTask.isPending ||
    updateParent.isPending
  const isError =
    createArea.isError ||
    createCategory.isError ||
    createSkill.isError ||
    createGoal.isError ||
    createTask.isError ||
    updateParent.isError

  const submit = async (values: CreateFormValues) => {
    const name = values.name.trim()
    const description = values.description.trim() || null
    const areaId = values.area_id === null ? null : Number(values.area_id)
    let id: number

    try {
      if (values.type === 'area') {
        const area: CreateDevelopmentArea = { name, description }
        id = (await createArea.mutateAsync(area)).id
      } else if (values.type === 'category') {
        const category: CreateDevelopmentCategory = {
          area_id: areaId as number,
          name,
        }
        id = (await createCategory.mutateAsync(category)).id
      } else if (values.type === 'skill') {
        const skill: CreateSkill = {
          area_id: areaId as number,
          state_id:
            values.state_id === null ? null : Number(values.state_id),
          name,
          description,
        }
        id = (await createSkill.mutateAsync(skill)).id
      } else if (values.type === 'goal') {
        const goal: CreateGoal = {
          area_id: areaId as number,
          name,
          description,
          due_at: values.due_at
            ? new Date(values.due_at).toISOString()
            : null,
        }
        id = (await createGoal.mutateAsync(goal)).id
      } else {
        const task: CreateTask = {
          area_id: areaId as number,
          group_id: null,
          name,
          description,
        }
        id = (await createTask.mutateAsync(task)).id
      }

      if (values.type !== 'area' && values.parent_id !== null) {
        await updateParent.mutateAsync({
          id,
          item: { parent_id: Number(values.parent_id) },
        })
      }

      onClose()
    } catch {
      return
    }
  }

  const metadataPending =
    allowedRelations.isPending ||
    (form.values.type === 'skill' && skillMetadata.isPending)
  const metadataError =
    allowedRelations.isError ||
    (form.values.type === 'skill' && skillMetadata.isError)

  return (
    <Modal centered opened onClose={onClose} title="Element hinzufügen">
      {metadataPending ? (
        <Center>
          <Loader />
        </Center>
      ) : metadataError ? (
        <Alert color="red" title="Fehler">
          Die Auswahldaten konnten nicht geladen werden.
        </Alert>
      ) : (
        <Box component="form" onSubmit={form.onSubmit(submit)}>
          <Stack>
            <Select
              label="Elementtyp"
              value={form.values.type}
              data={[
                { value: 'area', label: 'Development Area' },
                { value: 'category', label: 'Kategorie' },
                { value: 'skill', label: 'Fähigkeit' },
                { value: 'goal', label: 'Ziel' },
                { value: 'task', label: 'Aufgabe' },
              ]}
              onChange={(value) => {
                if (value !== null) {
                  form.setFieldValue('type', value as CreateEntityType)
                  form.setFieldValue('parent_id', null)
                }
              }}
            />
            <TextInput
              data-autofocus
              required
              label="Name"
              {...form.getInputProps('name')}
            />
            {form.values.type !== 'area' && (
              <Select
                required
                label="Continuum-Bereich"
                value={form.values.area_id}
                data={areas.map((area) => ({
                  value: String(area.id),
                  label: area.name,
                }))}
                error={form.errors.area_id}
                onBlur={() => form.validateField('area_id')}
                onChange={(value) => {
                  form.setFieldValue('area_id', value)
                  form.setFieldValue('parent_id', null)
                }}
              />
            )}
            {form.values.type === 'skill' && (
              <Select
                clearable
                label="Status"
                data={(skillMetadata.data?.states ?? []).map((state) => ({
                  value: String(state.id),
                  label: state.name,
                }))}
                {...form.getInputProps('state_id')}
              />
            )}
            {form.values.type === 'goal' && (
              <TextInput
                label="Fällig am"
                type="datetime-local"
                {...form.getInputProps('due_at')}
              />
            )}
            {form.values.type !== 'category' && (
              <Textarea
                autosize
                label="Beschreibung"
                minRows={3}
                {...form.getInputProps('description')}
              />
            )}
            {form.values.type !== 'area' && (
              <Select
                clearable
                disabled={form.values.area_id === null}
                label="Übergeordnetes Element"
                data={parentOptions}
                {...form.getInputProps('parent_id')}
              />
            )}
            {isError && (
              <Alert color="red" title="Fehler">
                Das Element konnte nicht erstellt werden.
              </Alert>
            )}
            <Group justify="flex-end">
              <Button
                disabled={isPending}
                type="button"
                variant="default"
                onClick={onClose}
              >
                Abbrechen
              </Button>
              <Button loading={isPending} type="submit">
                Hinzufügen
              </Button>
            </Group>
          </Stack>
        </Box>
      )}
    </Modal>
  )
}

function DeleteContinuumModal({
  entity,
  onClose,
}: {
  entity: SelectedEntity
  onClose: () => void
}) {
  const deleteArea = useDeleteDevelopmentArea()
  const deleteCategory = useDeleteDevelopmentCategory()
  const deleteSkill = useDeleteSkill()
  const deleteGoal = useDeleteGoal()
  const deleteTask = useDeleteTask()
  const isPending =
    deleteArea.isPending ||
    deleteCategory.isPending ||
    deleteSkill.isPending ||
    deleteGoal.isPending ||
    deleteTask.isPending
  const isError =
    deleteArea.isError ||
    deleteCategory.isError ||
    deleteSkill.isError ||
    deleteGoal.isError ||
    deleteTask.isError

  const remove = () => {
    if (entity.type === 'area') {
      deleteArea.mutate(entity.value.id, { onSuccess: onClose })
    } else if (entity.type === 'category') {
      deleteCategory.mutate(entity.value.item_id, { onSuccess: onClose })
    } else if (entity.type === 'skill') {
      deleteSkill.mutate(entity.value.item_id, { onSuccess: onClose })
    } else if (entity.type === 'goal') {
      deleteGoal.mutate(entity.value.item_id, { onSuccess: onClose })
    } else {
      deleteTask.mutate(entity.value.item_id, { onSuccess: onClose })
    }
  }

  return (
    <Modal centered opened onClose={onClose} title="Element löschen">
      <Stack>
        <Text>Möchtest du „{entity.value.name}“ wirklich löschen?</Text>
        {isError && (
          <Alert color="red" title="Fehler">
            Das Element konnte nicht gelöscht werden. Möglicherweise bestehen
            noch Verknüpfungen.
          </Alert>
        )}
        <Group justify="flex-end">
          <Button
            disabled={isPending}
            type="button"
            variant="default"
            onClick={onClose}
          >
            Abbrechen
          </Button>
          <Button color="red" loading={isPending} onClick={remove}>
            Löschen
          </Button>
        </Group>
      </Stack>
    </Modal>
  )
}

export function ContinuumFlow() {
  const [selectedEntity, setSelectedEntity] = useState<SelectedEntity | null>(
    null,
  )
  const [createOpened, setCreateOpened] = useState(false)
  const [entityToDelete, setEntityToDelete] = useState<SelectedEntity | null>(
    null,
  )
  const areas = useDevelopmentAreas()
  const categories = useDevelopmentCategories()
  const skills = useSkills()
  const goals = useGoals()
  const tasks = useTasks()
  const hierarchy = useItemHierarchy()
  const skillMetadata = useSkillMetadata()
  const taskMetadata = useTaskMetadata()
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
        skillMetadata.data?.states ?? [],
        taskMetadata.data?.states ?? [],
      ),
    [
      areas.data,
      categories.data,
      goals.data,
      hierarchy.data,
      skillMetadata.data,
      skills.data,
      taskMetadata.data,
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

  return (
    <Stack h="100%" mih={0}>
      <Group justify="space-between">
        <Title>Continuum</Title>
        <Button onClick={() => setCreateOpened(true)}>Hinzufügen</Button>
      </Group>
      <Box mih={0} style={{ flex: 1 }}>
        {flow.nodes.length === 0 ? (
          <Text>Keine Continuum-Daten vorhanden.</Text>
        ) : (
          <ReactFlow
            edges={flow.edges}
            elementsSelectable={false}
            fitView
            fitViewOptions={{ padding: 0.2 }}
            nodes={flow.nodes}
            nodeTypes={nodeTypes}
            nodesConnectable={false}
            nodesDraggable={false}
            onNodeClick={(_, node) => setSelectedEntity(node.data.entity)}
          >
            <Background />
            <ViewportPortal>
              {flow.separators.map((separator) => (
                <Box
                  key={separator.x}
                  aria-hidden
                  style={{
                    position: 'absolute',
                    height: separator.height,
                    borderLeft: '1px solid var(--mantine-color-gray-4)',
                    opacity: 0.6,
                    pointerEvents: 'none',
                    transform: `translate(${separator.x}px, ${separator.y}px)`,
                  }}
                />
              ))}
            </ViewportPortal>
            <Controls showInteractive={false} />
          </ReactFlow>
        )}
      </Box>
      {selectedEntity !== null && (
        <ContinuumEditModal
          areaName={selectedAreaName}
          categories={categories.data ?? []}
          entity={selectedEntity}
          goals={goals.data ?? []}
          hierarchy={hierarchy.data ?? []}
          skills={skills.data ?? []}
          tasks={tasks.data ?? []}
          onClose={() => setSelectedEntity(null)}
          onDelete={(entity) => {
            setSelectedEntity(null)
            setEntityToDelete(entity)
          }}
        />
      )}
      {createOpened && (
        <CreateContinuumModal
          areas={areas.data ?? []}
          categories={categories.data ?? []}
          goals={goals.data ?? []}
          skills={skills.data ?? []}
          tasks={tasks.data ?? []}
          onClose={() => setCreateOpened(false)}
        />
      )}
      {entityToDelete !== null && (
        <DeleteContinuumModal
          entity={entityToDelete}
          onClose={() => setEntityToDelete(null)}
        />
      )}
    </Stack>
  )
}
