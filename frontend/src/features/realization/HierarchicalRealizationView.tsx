import { useMemo, useState } from 'react'
import {
  ActionIcon,
  Alert,
  Box,
  Button,
  Center,
  ColorInput,
  Group,
  Loader,
  Menu,
  Modal,
  NumberInput,
  Paper,
  Select,
  Stack,
  Text,
  TextInput,
  ThemeIcon,
  Title,
} from '@mantine/core'
import { useForm } from '@mantine/form'
import {
  IconChevronDown,
  IconChevronRight,
  IconCards,
  IconDotsVertical,
  IconFileText,
  IconFolder,
  IconPencil,
  IconPlus,
  IconSparkles,
  IconTrash,
} from '@tabler/icons-react'
import { Group as PanelGroup, Panel, Separator as PanelSeparator } from 'react-resizable-panels'
import { useMetadata } from '../metadata/api'
import {
  useCreateDevelopmentArea,
  useCreateTopic,
  useDeleteDevelopmentArea,
  useDeleteEntity,
  useTopicGraph,
  useUpdateDevelopmentArea,
  useUpdateTopic,
} from './api'
import { FlashcardDialog, TopicDocumentPanel } from './RealizationView'
import type {
  CreateDevelopmentArea,
  DevelopmentArea,
  Topic,
  TopicGraphEntry,
} from './types'
import './RealizationView.css'

interface AreaFormValues {
  background_color: string
  banner_image_path: string
  icon: string
  name: string
  sort_order: number | string
}

interface TopicFormValues {
  name: string
  parent: string | null
}

interface TopicDialogState {
  areaId: number | null
  parentTopicId: number | null
  topic: Topic | null
}

interface DeleteTarget {
  id: number
  kind: 'area' | 'document' | 'topic'
  name: string
}

function DevelopmentAreaDialog({
  area,
  onClose,
}: {
  area: DevelopmentArea | null
  onClose: () => void
}) {
  const createArea = useCreateDevelopmentArea()
  const updateArea = useUpdateDevelopmentArea()
  const form = useForm<AreaFormValues>({
    initialValues: {
      background_color: area?.background_color ?? '',
      banner_image_path: area?.banner_image_path ?? '',
      icon: area?.icon ?? '',
      name: area?.name ?? '',
      sort_order: area?.sort_order ?? 0,
    },
    validate: {
      name: (value) => value.trim() === '' ? 'Name ist erforderlich.' : null,
    },
  })
  const isSaving = createArea.isPending || updateArea.isPending

  const submit = (values: AreaFormValues) => {
    const payload: CreateDevelopmentArea = {
      background_color: values.background_color.trim() || null,
      banner_image_path: values.banner_image_path.trim() || null,
      icon: values.icon.trim() || null,
      name: values.name.trim(),
      sort_order: Number(values.sort_order),
    }

    if (area === null) {
      createArea.mutate(payload, { onSuccess: onClose })
      return
    }

    updateArea.mutate({ area: payload, id: area.id }, { onSuccess: onClose })
  }

  return (
    <Modal
      centered
      opened
      title={area === null ? 'Bereich erstellen' : 'Bereich bearbeiten'}
      onClose={onClose}
    >
      <Box component="form" onSubmit={form.onSubmit(submit)}>
        <Stack gap="sm">
          <TextInput data-autofocus required label="Name" {...form.getInputProps('name')} />
          <ColorInput label="Hintergrundfarbe" {...form.getInputProps('background_color')} />
          <TextInput label="Banner-Pfad" {...form.getInputProps('banner_image_path')} />
          <TextInput label="Icon" {...form.getInputProps('icon')} />
          <NumberInput label="Sortierung" {...form.getInputProps('sort_order')} />
          {(createArea.isError || updateArea.isError) && (
            <Alert color="red" title="Fehler">
              Der Bereich konnte nicht gespeichert werden.
            </Alert>
          )}
          <Group justify="flex-end">
            <Button disabled={isSaving} type="button" variant="default" onClick={onClose}>
              Abbrechen
            </Button>
            <Button loading={isSaving} type="submit">Speichern</Button>
          </Group>
        </Stack>
      </Box>
    </Modal>
  )
}

function TopicDialog({
  areas,
  entries,
  state,
  onClose,
}: {
  areas: DevelopmentArea[]
  entries: TopicGraphEntry[]
  state: TopicDialogState
  onClose: () => void
}) {
  const createTopic = useCreateTopic()
  const updateTopic = useUpdateTopic()
  const topic = state.topic
  const form = useForm<TopicFormValues>({
    initialValues: {
      name: topic?.name ?? '',
      parent: topic?.parent_topic_entity_id !== null && topic !== null
        ? `topic:${topic.parent_topic_entity_id}`
        : topic?.development_area_id !== null && topic !== null
          ? `area:${topic.development_area_id}`
          : state.parentTopicId !== null
            ? `topic:${state.parentTopicId}`
            : state.areaId !== null
              ? `area:${state.areaId}`
              : null,
    },
    validate: {
      name: (value) => value.trim() === '' ? 'Name ist erforderlich.' : null,
      parent: (value) => value === null ? 'Übergeordnetes Element ist erforderlich.' : null,
    },
  })
  const unavailableParentIds = new Set<number>()
  if (topic !== null) {
    unavailableParentIds.add(topic.entity_id)
    let changed = true
    while (changed) {
      changed = false
      entries.forEach((entry) => {
        if (
          entry.topic.parent_topic_entity_id !== null &&
          unavailableParentIds.has(entry.topic.parent_topic_entity_id) &&
          !unavailableParentIds.has(entry.topic.entity_id)
        ) {
          unavailableParentIds.add(entry.topic.entity_id)
          changed = true
        }
      })
    }
  }
  const parentOptions = [
    ...areas.map((area) => ({ label: `Bereich: ${area.name}`, value: `area:${area.id}` })),
    ...entries
      .filter((entry) => !unavailableParentIds.has(entry.topic.entity_id))
      .map((entry) => ({
        label: `Topic: ${entry.topic.name}`,
        value: `topic:${entry.topic.entity_id}`,
      })),
  ]
  const isSaving = createTopic.isPending || updateTopic.isPending

  const submit = (values: TopicFormValues) => {
    if (values.parent === null) return
    const [kind, rawId] = values.parent.split(':')
    const parentId = Number(rawId)
    const payload = {
      development_area_id: kind === 'area' ? parentId : null,
      name: values.name.trim(),
      parent_topic_entity_id: kind === 'topic' ? parentId : null,
    }

    if (topic === null) {
      createTopic.mutate(payload, { onSuccess: onClose })
      return
    }

    updateTopic.mutate(
      {
        id: topic.entity_id,
        topic: { ...payload, understanding_level_id: topic.understanding_level_id },
      },
      { onSuccess: onClose },
    )
  }

  return (
    <Modal
      centered
      opened
      title={topic === null ? 'Topic erstellen' : 'Topic bearbeiten'}
      onClose={onClose}
    >
      <Box component="form" onSubmit={form.onSubmit(submit)}>
        <Stack gap="sm">
          <TextInput data-autofocus required label="Name" {...form.getInputProps('name')} />
          <Select
            required
            data={parentOptions}
            label="Übergeordnet"
            searchable
            {...form.getInputProps('parent')}
          />
          {(createTopic.isError || updateTopic.isError) && (
            <Alert color="red" title="Fehler">
              Das Topic konnte nicht gespeichert werden.
            </Alert>
          )}
          <Group justify="flex-end">
            <Button disabled={isSaving} type="button" variant="default" onClick={onClose}>
              Abbrechen
            </Button>
            <Button loading={isSaving} type="submit">Speichern</Button>
          </Group>
        </Stack>
      </Box>
    </Modal>
  )
}

function DeleteDialog({
  target,
  onClose,
  onDeleted,
}: {
  target: DeleteTarget
  onClose: () => void
  onDeleted: () => void
}) {
  const deleteArea = useDeleteDevelopmentArea()
  const deleteEntity = useDeleteEntity()
  const mutation = target.kind === 'area' ? deleteArea : deleteEntity

  const remove = () => {
    mutation.mutate(target.id, {
      onSuccess: () => {
        onDeleted()
        onClose()
      },
    })
  }

  return (
    <Modal centered opened title="Löschen" onClose={onClose}>
      <Stack>
        <Text>Möchtest du „{target.name}“ wirklich löschen?</Text>
        {mutation.isError && (
          <Alert color="red" title="Fehler">
            Der Eintrag konnte nicht gelöscht werden. Ein Bereich muss vor dem Löschen leer sein.
          </Alert>
        )}
        <Group justify="flex-end">
          <Button disabled={mutation.isPending} variant="default" onClick={onClose}>
            Abbrechen
          </Button>
          <Button color="red" loading={mutation.isPending} onClick={remove}>
            Löschen
          </Button>
        </Group>
      </Stack>
    </Modal>
  )
}

function TopicTreeItem({
  childrenByParent,
  depth,
  entry,
  expanded,
  onCreateChild,
  onDelete,
  onEdit,
  onManageFlashcards,
  onSelect,
  onToggle,
  selectedTopicId,
}: {
  childrenByParent: Map<number, TopicGraphEntry[]>
  depth: number
  entry: TopicGraphEntry
  expanded: Set<number>
  onCreateChild: (topic: Topic) => void
  onDelete: (target: DeleteTarget) => void
  onEdit: (topic: Topic) => void
  onManageFlashcards: (entry: TopicGraphEntry) => void
  onSelect: (entry: TopicGraphEntry) => void
  onToggle: (id: number) => void
  selectedTopicId: number | null
}) {
  const topic = entry.topic
  const children = childrenByParent.get(topic.entity_id) ?? []
  const isExpanded = expanded.has(topic.entity_id)

  return (
    <>
      <Group
        className="realization-tree-row"
        data-selected={selectedTopicId === topic.entity_id || undefined}
        gap={6}
        justify="space-between"
        pl={8 + depth * 22}
        pr="xs"
        wrap="nowrap"
        onClick={() => onSelect(entry)}
      >
        <Group gap={6} miw={0} wrap="nowrap">
          {children.length === 0 ? (
            <Box w={28} />
          ) : (
            <ActionIcon
              aria-label={isExpanded ? 'Topic zuklappen' : 'Topic aufklappen'}
              size="sm"
              variant="subtle"
              onClick={(event) => {
                event.stopPropagation()
                onToggle(topic.entity_id)
              }}
            >
              {isExpanded ? <IconChevronDown size={15} /> : <IconChevronRight size={15} />}
            </ActionIcon>
          )}
          {entry.document === null ? <IconFolder size={17} /> : <IconFileText size={17} />}
          <Text fw={depth === 0 ? 600 : 400} lineClamp={1} size="sm">
            {topic.name}
          </Text>
        </Group>
        <Menu position="bottom-end" withinPortal>
          <Menu.Target>
            <ActionIcon
              aria-label="Topic-Aktionen"
              size="sm"
              variant="subtle"
              onClick={(event) => event.stopPropagation()}
            >
              <IconDotsVertical size={16} />
            </ActionIcon>
          </Menu.Target>
          <Menu.Dropdown onClick={(event) => event.stopPropagation()}>
            <Menu.Item leftSection={<IconPlus size={15} />} onClick={() => onCreateChild(topic)}>
              Unterthema erstellen
            </Menu.Item>
            <Menu.Item leftSection={<IconPencil size={15} />} onClick={() => onEdit(topic)}>
              Bearbeiten
            </Menu.Item>
            <Menu.Item
              disabled={entry.document === null}
              leftSection={<IconCards size={15} />}
              onClick={() => onManageFlashcards(entry)}
            >
              Lernkarten verwalten
            </Menu.Item>
            <Menu.Item
              color="red"
              leftSection={<IconTrash size={15} />}
              onClick={() => onDelete({ id: topic.entity_id, kind: 'topic', name: topic.name })}
            >
              Löschen
            </Menu.Item>
          </Menu.Dropdown>
        </Menu>
      </Group>

      {isExpanded && children.map((child) => (
          <TopicTreeItem
            key={child.topic.entity_id}
            childrenByParent={childrenByParent}
            depth={depth + 1}
            entry={child}
            expanded={expanded}
            onCreateChild={onCreateChild}
            onDelete={onDelete}
            onEdit={onEdit}
            onManageFlashcards={onManageFlashcards}
            onSelect={onSelect}
            onToggle={onToggle}
            selectedTopicId={selectedTopicId}
          />
      ))}
    </>
  )
}

export function RealizationView() {
  const graph = useTopicGraph()
  const metadata = useMetadata()
  const [areaDialog, setAreaDialog] = useState<DevelopmentArea | null | undefined>()
  const [topicDialog, setTopicDialog] = useState<TopicDialogState | null>(null)
  const [deleteTarget, setDeleteTarget] = useState<DeleteTarget | null>(null)
  const [flashcardEntry, setFlashcardEntry] = useState<TopicGraphEntry | null>(null)
  const [expanded, setExpanded] = useState(() => new Set<number>())
  const [selectedTopicId, setSelectedTopicId] = useState<number | null>(null)
  const [createDocumentTopicId, setCreateDocumentTopicId] = useState<number | null>(null)
  const [documentDrafts] = useState(() => new Map<number, string>())
  const entries = useMemo(() => graph.data?.topics ?? [], [graph.data?.topics])
  const selectedEntry = entries.find(({ topic }) => topic.entity_id === selectedTopicId)
  const selectedArea = graph.data?.development_areas.find(
    (area) => area.id === selectedEntry?.topic.development_area_id,
  )
  const selectedParent = entries.find(
    (entry) => entry.topic.entity_id === selectedEntry?.topic.parent_topic_entity_id,
  )
  const selectedUnderstanding = metadata.data?.topic_understanding_levels.find(
    (level) => level.id === selectedEntry?.topic.understanding_level_id,
  )
  const unassignedRoots = entries.filter(({ topic }) =>
    topic.parent_topic_entity_id === null && topic.development_area_id === null,
  )
  const childrenByParent = useMemo(() => {
    const children = new Map<number, TopicGraphEntry[]>()
    entries.forEach((entry) => {
      const parentId = entry.topic.parent_topic_entity_id
      if (parentId === null) return
      const siblings = children.get(parentId) ?? []
      siblings.push(entry)
      children.set(parentId, siblings)
    })
    return children
  }, [entries])

  const toggleTopic = (id: number) => {
    setExpanded((current) => {
      const next = new Set(current)
      if (next.has(id)) next.delete(id)
      else next.add(id)
      return next
    })
  }
  const createChild = (topic: Topic) => {
    setExpanded((current) => new Set(current).add(topic.entity_id))
    setTopicDialog({
      areaId: null,
      parentTopicId: topic.entity_id,
      topic: null,
    })
  }
  const editTopic = (topic: Topic) => setTopicDialog({
    areaId: null,
    parentTopicId: null,
    topic,
  })
  const selectTopic = (entry: TopicGraphEntry) => {
    setSelectedTopicId(entry.topic.entity_id)
    setCreateDocumentTopicId(null)
  }

  if (graph.isPending) {
    return <Center h="100%"><Loader /></Center>
  }

  if (graph.isError) {
    return (
      <Alert color="red" title="Fehler">
        Die Selbstverwirklichungsdaten konnten nicht geladen werden.
      </Alert>
    )
  }

  return (
    <Box h="100%" w="100%">
      <PanelGroup
        defaultLayout={{ editor: 55, hierarchy: 25, properties: 20 }}
        orientation="horizontal"
        style={{ height: '100%', width: '100%' }}
      >
        <Panel id="hierarchy" minSize="18%">
          <Box className="realization-hierarchy-scroll">
            <Box p="sm">
              <Group justify="space-between" mb="sm">
                <Title order={3}>Verwirklichung</Title>
                <ActionIcon aria-label="Bereich erstellen" variant="light" onClick={() => setAreaDialog(null)}>
                  <IconPlus size={17} />
                </ActionIcon>
              </Group>

              <Stack gap="sm">
                {graph.data.development_areas.map((area) => {
                  const roots = entries.filter(({ topic }) =>
                    topic.parent_topic_entity_id === null &&
                    topic.development_area_id === area.id,
                  )
                  return (
                    <Paper key={area.id} className="realization-area" withBorder>
                      <Box
                        className="realization-area-banner realization-area-banner-compact"
                        style={{
                          backgroundColor: area.background_color ?? 'var(--mantine-color-gray-1)',
                          backgroundImage: area.banner_image_path === null
                            ? undefined
                            : `linear-gradient(90deg, rgb(0 0 0 / 45%), rgb(0 0 0 / 10%)), url("${area.banner_image_path}")`,
                          color: area.banner_image_path === null ? undefined : 'white',
                        }}
                      >
                        <Group justify="space-between" wrap="nowrap">
                          <Group gap="xs" miw={0} wrap="nowrap">
                            <ThemeIcon color="gray" size="sm" variant="light">
                              {area.icon === null ? <IconSparkles size={15} /> : (
                                <Text fw={700} lineClamp={1} size="xs">{area.icon}</Text>
                              )}
                            </ThemeIcon>
                            <Text fw={600} lineClamp={1} size="sm">{area.name}</Text>
                          </Group>
                          <Menu position="bottom-end" withinPortal>
                            <Menu.Target>
                              <ActionIcon aria-label="Bereich-Aktionen" size="sm" variant="subtle">
                                <IconDotsVertical size={16} />
                              </ActionIcon>
                            </Menu.Target>
                            <Menu.Dropdown>
                              <Menu.Item
                                leftSection={<IconPlus size={15} />}
                                onClick={() => setTopicDialog({ areaId: area.id, parentTopicId: null, topic: null })}
                              >
                                Root-Topic erstellen
                              </Menu.Item>
                              <Menu.Item leftSection={<IconPencil size={15} />} onClick={() => setAreaDialog(area)}>
                                Bearbeiten
                              </Menu.Item>
                              <Menu.Item
                                color="red"
                                leftSection={<IconTrash size={15} />}
                                onClick={() => setDeleteTarget({ id: area.id, kind: 'area', name: area.name })}
                              >
                                Löschen
                              </Menu.Item>
                            </Menu.Dropdown>
                          </Menu>
                        </Group>
                      </Box>
                      <Box py={3}>
                        {roots.length === 0 ? (
                          <Text c="dimmed" px="sm" py={6} size="xs">Keine Root-Topics.</Text>
                        ) : roots.map((entry) => (
                          <TopicTreeItem
                            key={entry.topic.entity_id}
                            childrenByParent={childrenByParent}
                            depth={0}
                            entry={entry}
                            expanded={expanded}
                            onCreateChild={createChild}
                            onDelete={setDeleteTarget}
                            onEdit={editTopic}
                            onManageFlashcards={setFlashcardEntry}
                            onSelect={selectTopic}
                            onToggle={toggleTopic}
                            selectedTopicId={selectedTopicId}
                          />
                        ))}
                      </Box>
                    </Paper>
                  )
                })}

                {unassignedRoots.length > 0 && (
                  <Paper className="realization-area" withBorder>
                    <Box className="realization-area-banner realization-area-banner-compact">
                      <Group gap="xs">
                        <ThemeIcon color="gray" size="sm" variant="light"><IconFolder size={15} /></ThemeIcon>
                        <Text fw={600} size="sm">Ohne Bereich</Text>
                      </Group>
                    </Box>
                    <Box py={3}>
                      {unassignedRoots.map((entry) => (
                        <TopicTreeItem
                          key={entry.topic.entity_id}
                          childrenByParent={childrenByParent}
                          depth={0}
                          entry={entry}
                          expanded={expanded}
                          onCreateChild={createChild}
                          onDelete={setDeleteTarget}
                          onEdit={editTopic}
                          onManageFlashcards={setFlashcardEntry}
                          onSelect={selectTopic}
                          onToggle={toggleTopic}
                          selectedTopicId={selectedTopicId}
                        />
                      ))}
                    </Box>
                  </Paper>
                )}
              </Stack>
            </Box>
          </Box>
        </Panel>

        <PanelSeparator className="realization-panel-separator" />
        <Panel id="editor" minSize="35%">
          {selectedEntry === undefined ? (
            <Center h="100%"><Text c="dimmed">Wähle ein Topic aus.</Text></Center>
          ) : selectedEntry.document === null && createDocumentTopicId !== selectedEntry.topic.entity_id ? (
            <Center h="100%">
              <Stack align="center" gap="sm">
                <Text c="dimmed">Für dieses Topic gibt es noch kein Dokument.</Text>
                <Button onClick={() => setCreateDocumentTopicId(selectedEntry.topic.entity_id)}>
                  Dokument erstellen
                </Button>
              </Stack>
            </Center>
          ) : (
            <TopicDocumentPanel
              key={selectedEntry.topic.entity_id}
              document={selectedEntry.document}
              drafts={documentDrafts}
              topic={selectedEntry.topic}
              onClose={() => setSelectedTopicId(null)}
            />
          )}
        </Panel>

        <PanelSeparator className="realization-panel-separator" />
        <Panel id="properties" minSize="18%">
          <Paper h="100%" p="md" radius={0} withBorder>
            <Title mb="md" order={4}>Eigenschaften</Title>
            {selectedEntry === undefined ? (
              <Text c="dimmed" size="sm">Kein Topic ausgewählt.</Text>
            ) : (
              <Stack gap="md">
                <Box>
                  <Text c="dimmed" size="xs">Name</Text>
                  <Text size="sm">{selectedEntry.topic.name}</Text>
                </Box>
                <Box>
                  <Text c="dimmed" size="xs">Übergeordnet</Text>
                  <Text size="sm">
                    {selectedParent?.topic.name ?? selectedArea?.name ?? 'Ohne Bereich'}
                  </Text>
                </Box>
                <Box>
                  <Text c="dimmed" size="xs">Verständnisgrad</Text>
                  <Text size="sm">
                    {selectedUnderstanding?.display_name ?? selectedUnderstanding?.name ?? '–'}
                  </Text>
                </Box>
                <Button
                  leftSection={<IconPencil size={15} />}
                  size="compact-sm"
                  variant="default"
                  onClick={() => editTopic(selectedEntry.topic)}
                >
                  Bearbeiten
                </Button>
              </Stack>
            )}
          </Paper>
        </Panel>
      </PanelGroup>

      {flashcardEntry?.document !== null && flashcardEntry !== null && (
        <FlashcardDialog
          document={flashcardEntry.document}
          flashcards={flashcardEntry.flashcards}
          onClose={() => setFlashcardEntry(null)}
        />
      )}
      {areaDialog !== undefined && (
        <DevelopmentAreaDialog area={areaDialog} onClose={() => setAreaDialog(undefined)} />
      )}
      {topicDialog !== null && (
        <TopicDialog
          areas={graph.data.development_areas}
          entries={entries}
          state={topicDialog}
          onClose={() => setTopicDialog(null)}
        />
      )}
      {deleteTarget !== null && (
        <DeleteDialog
          target={deleteTarget}
          onClose={() => setDeleteTarget(null)}
          onDeleted={() => {
            if (
              deleteTarget.kind === 'topic' && deleteTarget.id === selectedTopicId ||
              deleteTarget.kind === 'document' &&
              deleteTarget.id === selectedEntry?.document?.entity_id
            ) setSelectedTopicId(null)
          }}
        />
      )}
    </Box>
  )
}
