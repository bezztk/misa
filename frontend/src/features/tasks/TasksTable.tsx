import { useMemo, useState } from 'react'
import {
  Alert,
  Badge,
  Box,
  Button,
  Center,
  Divider,
  Group,
  Loader,
  Modal,
  Select,
  Stack,
  Table,
  Text,
  Textarea,
  TextInput,
  Title,
  UnstyledButton,
} from '@mantine/core'
import { useForm } from '@mantine/form'
import {
  columnFilteringFeature,
  createColumnHelper,
  createFilteredRowModel,
  createSortedRowModel,
  filterFn_equals,
  rowSortingFeature,
  sortFn_basic,
  sortFn_text,
  tableFeatures,
  useTable,
} from '@tanstack/react-table'
import { useNavigate, useSearch } from '@tanstack/react-router'
import {
  useCreateTask,
  useDeleteTask,
  useTasks,
  useUpdateTask,
} from './api'
import { CalendarEntryModal } from '../calendar/CalendarView'
import { useMetadata } from '../metadata/api'
import type { CreateTask, Task, UpdateTask } from './types'

const features = tableFeatures({
  columnFilteringFeature,
  rowSortingFeature,
  filteredRowModel: createFilteredRowModel(),
  sortedRowModel: createSortedRowModel(),
  filterFns: { equals: filterFn_equals },
  sortFns: { basic: sortFn_basic, text: sortFn_text },
})

const columnHelper = createColumnHelper<typeof features, Task>()
const dateFormatter = new Intl.DateTimeFormat('de-DE')
const emptyTasks: Task[] = []

type TaskFormState = { mode: 'create' } | { mode: 'edit'; task: Task }

const statusColors: Record<string, string> = {
  open: 'gray',
  in_progress: 'blue',
  completed: 'green',
  cancelled: 'red',
}

interface TaskFormValues {
  name: string
  description: string
  group_id: string | null
  state_id: string | null
  priority_id: string | null
  due_at: string
}

function optionalString(value: number | null | undefined) {
  return value === null || value === undefined ? null : String(value)
}

function toLocalDateTime(value: string | null | undefined) {
  if (!value) return ''

  const date = new Date(value)
  const offset = date.getTimezoneOffset() * 60_000
  return new Date(date.getTime() - offset).toISOString().slice(0, 16)
}

function TaskFormModal({
  state,
  onClose,
  onDelete,
}: {
  state: TaskFormState
  onClose: () => void
  onDelete: (task: Task) => void
}) {
  const task = state.mode === 'edit' ? state.task : null
  const metadata = useMetadata()
  const createTask = useCreateTask()
  const updateTask = useUpdateTask()
  const form = useForm<TaskFormValues>({
    initialValues: {
      name: task?.name ?? '',
      description: task?.description ?? '',
      group_id: task === null ? '1' : optionalString(task.group_id),
      state_id: task === null ? null : String(task.state_id),
      priority_id: task === null ? '2' : optionalString(task.priority_id),
      due_at: toLocalDateTime(task?.due_at),
    },
    validate: {
      name: (value) => {
        const length = [...value.trim()].length

        if (length === 0) return 'Name ist erforderlich.'
        if (length > 150) return 'Der Name darf maximal 150 Zeichen haben.'
        return null
      },
      state_id: (value) =>
        state.mode === 'edit' && value === null
          ? 'Status ist erforderlich.'
          : null,
      group_id: (value) =>
        value === null ? 'Gruppe ist erforderlich.' : null,
      priority_id: (value) =>
        value === null ? 'Priorität ist erforderlich.' : null,
    },
  })

  const isLoading = metadata.isPending
  const isError = metadata.isError
  const isSaving = createTask.isPending || updateTask.isPending

  const submit = (values: TaskFormValues) => {
    const name = values.name.trim()
    const group_id = Number(values.group_id)
    const priority_id = Number(values.priority_id)
    const due_at = values.due_at
      ? new Date(values.due_at).toISOString()
      : null

    if (task === null) {
      const newTask: CreateTask = {
        name,
        description: values.description.trim() || null,
        group_id,
        priority_id,
        due_at,
      }

      createTask.mutate(newTask, { onSuccess: onClose })
      return
    }

    const changedTask: UpdateTask = {
      name,
      description: values.description.trim() || null,
      group_id,
      state_id:
        values.state_id === null ? task.state_id : Number(values.state_id),
      priority_id,
      due_at,
    }

    updateTask.mutate(
      { entityId: task.entity_id, task: changedTask },
      { onSuccess: onClose },
    )
  }

  return (
    <Modal
      centered
      opened
      onClose={onClose}
      title={task === null ? 'Aufgabe erstellen' : 'Aufgabe bearbeiten'}
    >
      {isLoading ? (
        <Center>
          <Loader />
        </Center>
      ) : isError ? (
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
            <Select
              required
              label="Gruppe"
              data={(metadata.data?.task_groups ?? []).map((group) => ({
                value: String(group.id),
                label: group.name,
              }))}
              {...form.getInputProps('group_id')}
            />
            {task !== null && (
              <Select
                required
                label="Status"
                data={(metadata.data?.task_states ?? []).map((taskState) => ({
                  value: String(taskState.id),
                  label: taskState.display_name ?? taskState.name,
                }))}
                {...form.getInputProps('state_id')}
              />
            )}
            <Select
              required
              label="Priorität"
              data={(metadata.data?.task_priorities ?? []).map((priority) => ({
                value: String(priority.id),
                label: priority.display_name ?? priority.name,
              }))}
              {...form.getInputProps('priority_id')}
            />
            <TextInput
              label="Fällig am"
              type="datetime-local"
              {...form.getInputProps('due_at')}
            />
            <Textarea
              autosize
              label="Beschreibung"
              minRows={3}
              {...form.getInputProps('description')}
            />
            {(createTask.isError || updateTask.isError) && (
              <Alert color="red" title="Fehler">
                Die Aufgabe konnte nicht gespeichert werden.
              </Alert>
            )}
            <Group justify={task === null ? 'flex-end' : 'space-between'}>
              {task !== null && (
                <Button
                  color="red"
                  disabled={isSaving}
                  type="button"
                  variant="light"
                  onClick={() => onDelete(task)}
                >
                  Löschen
                </Button>
              )}
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

function DeleteTaskModal({ task, onClose }: { task: Task; onClose: () => void }) {
  const deleteTask = useDeleteTask()

  return (
    <Modal centered opened onClose={onClose} title="Aufgabe löschen">
      <Stack>
        <Text>Möchtest du „{task.name}“ wirklich löschen?</Text>
        {deleteTask.isError && (
          <Alert color="red" title="Fehler">
            Die Aufgabe konnte nicht gelöscht werden.
          </Alert>
        )}
        <Group justify="flex-end">
          <Button
            disabled={deleteTask.isPending}
            type="button"
            variant="default"
            onClick={onClose}
          >
            Abbrechen
          </Button>
          <Button
            color="red"
            loading={deleteTask.isPending}
            onClick={() =>
              deleteTask.mutate(task.entity_id, { onSuccess: onClose })
            }
          >
            Löschen
          </Button>
        </Group>
      </Stack>
    </Modal>
  )
}

export function TasksTable() {
  const navigate = useNavigate()
  const { taskId } = useSearch({ from: '/tasks' })
  const tasks = useTasks()
  const metadata = useMetadata()
  const [formState, setFormState] = useState<TaskFormState | null>(null)
  const [taskToDelete, setTaskToDelete] = useState<Task | null>(null)
  const [taskToSchedule, setTaskToSchedule] = useState<Task | null>(null)
  const routedTask = tasks.data?.find(
    (candidate) => candidate.entity_id === taskId,
  )
  const activeFormState =
    formState ??
    (routedTask === undefined
      ? null
      : ({ mode: 'edit', task: routedTask } as const))

  const closeTaskForm = () => {
    setFormState(null)
    if (taskId !== undefined) {
      void navigate({ to: '/tasks', search: {}, replace: true })
    }
  }
  const statusesById = useMemo(
    () => new Map(metadata.data?.task_states.map((state) => [state.id, state])),
    [metadata.data?.task_states],
  )
  const groupsById = useMemo(
    () => new Map(metadata.data?.task_groups.map((group) => [group.id, group])),
    [metadata.data?.task_groups],
  )
  const columns = useMemo(
    () =>
      columnHelper.columns([
        columnHelper.accessor('name', {
          header: 'Aufgabe',
          cell: ({ getValue }) => <Text fw={500}>{getValue()}</Text>,
          sortFn: 'text',
        }),
        columnHelper.accessor('description', {
          header: 'Beschreibung',
          cell: ({ getValue }) => (
            <Text lineClamp={1} maw={400} title={getValue() ?? undefined}>
              {getValue() || '–'}
            </Text>
          ),
          enableSorting: false,
        }),
        columnHelper.accessor('state_id', {
          header: 'Status',
          cell: ({ getValue }) => {
            const status = statusesById.get(getValue())

            return status === undefined ? (
              <Text>–</Text>
            ) : (
              <Badge color={statusColors[status.name] ?? 'gray'} variant="light">
                {status.display_name ?? status.name}
              </Badge>
            )
          },
          filterFn: 'equals',
          sortFn: 'basic',
        }),
        columnHelper.accessor('group_id', {
          header: 'Gruppe',
          cell: ({ getValue }) => {
            const groupId = getValue()
            return groupsById.get(groupId)?.name ?? '–'
          },
          filterFn: 'equals',
          sortFn: 'basic',
        }),
        columnHelper.accessor('created_at', {
          header: 'Erstellt',
          cell: ({ getValue }) => dateFormatter.format(new Date(getValue())),
          sortFn: 'basic',
        }),
        columnHelper.display({
          id: 'schedule',
          header: 'Kalender',
          cell: ({ row }) => (
            <Button
              size="xs"
              variant="light"
              onClick={(event) => {
                event.stopPropagation()
                setTaskToSchedule(row.original)
              }}
              onKeyDown={(event) => event.stopPropagation()}
            >
              Einplanen
            </Button>
          ),
        }),
      ]),
    [groupsById, statusesById],
  )
  const table = useTable({
    features,
    columns,
    data: tasks.data ?? emptyTasks,
  })
  const statusFilter = table.getColumn('state_id')?.getFilterValue() as
    | number
    | undefined
  const groupFilter = table.getColumn('group_id')?.getFilterValue() as
    | number
    | undefined
  const rows = table.getRowModel().rows

  return (
    <Stack gap="md">
      <Box>
        <Group mih={40} justify="space-between">
          <Title>Aufgaben</Title>
          <Button onClick={() => setFormState({ mode: 'create' })}>
            Neue Aufgabe
          </Button>
        </Group>
        <Divider mt="md" />
      </Box>

      {tasks.isPending || metadata.isPending ? (
        <Center>
          <Loader />
        </Center>
      ) : tasks.isError || metadata.isError ? (
        <Alert color="red" title="Fehler">
          Die Aufgaben oder Auswahldaten konnten nicht geladen werden.
        </Alert>
      ) : tasks.data.length === 0 ? (
        <Text>Keine Aufgaben vorhanden.</Text>
      ) : (
        <>
          <Group grow maw={600}>
            <Select
              label="Statusfilter"
              value={
                statusFilter === undefined ? 'all' : String(statusFilter)
              }
              data={[
                { value: 'all', label: 'Alle Status' },
                ...(metadata.data?.task_states ?? []).map((status) => ({
                  value: String(status.id),
                  label: status.display_name ?? status.name,
                })),
              ]}
              onChange={(value) =>
                table
                  .getColumn('state_id')
                  ?.setFilterValue(
                    value === null || value === 'all'
                      ? undefined
                      : Number(value),
                  )
              }
            />
            <Select
              label="Gruppenfilter"
              value={groupFilter === undefined ? 'all' : String(groupFilter)}
              data={[
                { value: 'all', label: 'Alle Gruppen' },
                ...(metadata.data?.task_groups ?? []).map((group) => ({
                  value: String(group.id),
                  label: group.name,
                })),
              ]}
              onChange={(value) =>
                table
                  .getColumn('group_id')
                  ?.setFilterValue(
                    value === null || value === 'all'
                      ? undefined
                      : Number(value),
                  )
              }
            />
          </Group>

          {rows.length === 0 ? (
            <Text>Keine Aufgaben entsprechen den Filtern.</Text>
          ) : (
            <Table.ScrollContainer minWidth={700}>
              <Table striped highlightOnHover>
                <Table.Thead>
                  {table.getHeaderGroups().map((headerGroup) => (
                    <Table.Tr key={headerGroup.id}>
                      {headerGroup.headers.map((header) => {
                        const sortDirection = header.column.getIsSorted()

                        return (
                          <Table.Th
                            key={header.id}
                            aria-sort={
                              sortDirection === 'asc'
                                ? 'ascending'
                                : sortDirection === 'desc'
                                  ? 'descending'
                                  : 'none'
                            }
                          >
                            {header.isPlaceholder ? null : (
                              <UnstyledButton
                                disabled={!header.column.getCanSort()}
                                onClick={
                                  header.column.getToggleSortingHandler()
                                }
                              >
                                <Group gap="xs" wrap="nowrap">
                                  <table.FlexRender header={header} />
                                  <Text component="span" size="sm">
                                    {sortDirection === 'asc'
                                      ? '↑'
                                      : sortDirection === 'desc'
                                        ? '↓'
                                        : ''}
                                  </Text>
                                </Group>
                              </UnstyledButton>
                            )}
                          </Table.Th>
                        )
                      })}
                    </Table.Tr>
                  ))}
                </Table.Thead>
                <Table.Tbody>
                  {rows.map((row) => (
                    <Table.Tr
                      key={row.id}
                      tabIndex={0}
                      onClick={() =>
                        setFormState({ mode: 'edit', task: row.original })
                      }
                      onKeyDown={(event) => {
                        if (event.key === 'Enter' || event.key === ' ') {
                          event.preventDefault()
                          setFormState({ mode: 'edit', task: row.original })
                        }
                      }}
                      style={{ cursor: 'pointer' }}
                    >
                      {row.getAllCells().map((cell) => (
                        <Table.Td key={cell.id}>
                          <table.FlexRender cell={cell} />
                        </Table.Td>
                      ))}
                    </Table.Tr>
                  ))}
                </Table.Tbody>
              </Table>
            </Table.ScrollContainer>
          )}
        </>
      )}

      {activeFormState !== null && (
        <TaskFormModal
          state={activeFormState}
          onClose={closeTaskForm}
          onDelete={(task) => {
            setFormState(null)
            setTaskToDelete(task)
          }}
        />
      )}
      {taskToDelete !== null && (
        <DeleteTaskModal
          task={taskToDelete}
          onClose={() => {
            setTaskToDelete(null)
            if (taskId !== undefined) {
              void navigate({ to: '/tasks', search: {}, replace: true })
            }
          }}
        />
      )}
      {taskToSchedule !== null && (
        <CalendarEntryModal
          state={{
            mode: 'create',
            selectedTime: { start: '', end: '' },
            sourceEntityId: taskToSchedule.entity_id,
            title: taskToSchedule.name,
          }}
          onClose={() => setTaskToSchedule(null)}
        />
      )}
    </Stack>
  )
}
