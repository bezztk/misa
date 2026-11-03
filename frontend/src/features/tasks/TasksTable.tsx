import {
  Alert,
  Center,
  Group,
  Loader,
  Stack,
  Table,
  Text,
  Title,
  UnstyledButton,
} from '@mantine/core'
import {
  createColumnHelper,
  createSortedRowModel,
  rowSortingFeature,
  sortFn_basic,
  sortFn_text,
  tableFeatures,
  useTable,
} from '@tanstack/react-table'
import { useTasks } from './api'
import type { Task } from './types'

const features = tableFeatures({
  rowSortingFeature,
  sortedRowModel: createSortedRowModel(),
  sortFns: { basic: sortFn_basic, text: sortFn_text },
})

const columnHelper = createColumnHelper<typeof features, Task>()
const dateFormatter = new Intl.DateTimeFormat('de-DE')
const emptyTasks: Task[] = []

const columns = columnHelper.columns([
  columnHelper.accessor('name', { header: 'Aufgabe', sortFn: 'text' }),
  columnHelper.accessor('description', {
    header: 'Beschreibung',
    cell: ({ getValue }) => getValue() ?? '–',
    enableSorting: false,
  }),
  columnHelper.accessor('state_id', { header: 'Status-ID', sortFn: 'basic' }),
  columnHelper.accessor('group_id', {
    header: 'Gruppen-ID',
    cell: ({ getValue }) => getValue() ?? '–',
    sortFn: 'basic',
  }),
  columnHelper.accessor('area_id', {
    header: 'Bereichs-ID',
    cell: ({ getValue }) => getValue() ?? '–',
    sortFn: 'basic',
  }),
  columnHelper.accessor('created_at', {
    header: 'Erstellt',
    cell: ({ getValue }) => dateFormatter.format(new Date(getValue())),
    sortFn: 'basic',
  }),
])

export function TasksTable() {
  const tasks = useTasks()
  const table = useTable({
    features,
    columns,
    data: tasks.data ?? emptyTasks,
  })

  if (tasks.isPending) {
    return (
      <Stack>
        <Title>Aufgaben</Title>
        <Center>
          <Loader />
        </Center>
      </Stack>
    )
  }

  if (tasks.isError) {
    return (
      <Stack>
        <Title>Aufgaben</Title>
        <Alert color="red" title="Fehler">
          Die Aufgaben konnten nicht geladen werden.
        </Alert>
      </Stack>
    )
  }

  if (tasks.data.length === 0) {
    return (
      <Stack>
        <Title>Aufgaben</Title>
        <Text>Keine Aufgaben vorhanden.</Text>
      </Stack>
    )
  }

  return (
    <Stack>
      <Title>Aufgaben</Title>
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
                          onClick={header.column.getToggleSortingHandler()}
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
            {table.getRowModel().rows.map((row) => (
              <Table.Tr key={row.id}>
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
    </Stack>
  )
}
