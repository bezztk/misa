import { useMemo, useState } from 'react'
import {
  ActionIcon,
  Alert,
  Box,
  Button,
  Center,
  Collapse,
  ColorInput,
  Group,
  Loader,
  Modal,
  NumberInput,
  Paper,
  Select,
  Stack,
  Text,
  TextInput,
  ThemeIcon,
  Title,
  Tooltip,
} from '@mantine/core'
import { useForm } from '@mantine/form'
import {
  IconChevronDown,
  IconChevronRight,
  IconFileText,
  IconFolder,
  IconFolderPlus,
  IconPencil,
  IconPlus,
  IconSparkles,
  IconTrash,
} from '@tabler/icons-react'
import { Group as PanelGroup, Panel, Separator as PanelSeparator } from 'react-resizable-panels'
import {
  useCreateDevelopmentArea,
  useCreateTopic,
  useDeleteDevelopmentArea,
  useDeleteEntity,
  useTopicGraph,
  useUpdateDevelopmentArea,
  useUpdateTopic,
} from './api'
import { TopicDocumentPanel } from './RealizationView'
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
  onOpenDocument,
  onToggle,
}: {
  childrenByParent: Map<number, TopicGraphEntry[]>
  depth: number
  entry: TopicGraphEntry
  expanded: Set<number>
  onCreateChild: (topic: Topic) => void
  onDelete: (target: DeleteTarget) => void
  onEdit: (topic: Topic) => void
  onOpenDocument: (entry: TopicGraphEntry) => void
  onToggle: (id: number) => void
}) {
  const topic = entry.topic
  const children = childrenByParent.get(topic.entity_id) ?? []
  const isExpanded = expanded.has(topic.entity_id)

  return (
    <>
      <Group
        className="realization-tree-row"
        gap={6}
        justify="space-between"
        pl={8 + depth * 22}
        pr="xs"
        wrap="nowrap"
      >
        <Group gap={6} miw={0} wrap="nowrap">
          <ActionIcon
            aria-label={isExpanded ? 'Topic zuklappen' : 'Topic aufklappen'}
            size="sm"
            variant="subtle"
            onClick={() => onToggle(topic.entity_id)}
          >
            {isExpanded ? <IconChevronDown size={15} /> : <IconChevronRight size={15} />}
          </ActionIcon>
          <IconFolder size={17} />
          <Text fw={depth === 0 ? 600 : 400} lineClamp={1} size="sm">
            {topic.name}
          </Text>
        </Group>
        <Group gap={2} wrap="nowrap">
          <Tooltip label="Untertopic erstellen">
            <ActionIcon size="sm" variant="subtle" onClick={() => onCreateChild(topic)}>
              <IconFolderPlus size={15} />
            </ActionIcon>
          </Tooltip>
          <Tooltip label="Topic bearbeiten">
            <ActionIcon size="sm" variant="subtle" onClick={() => onEdit(topic)}>
              <IconPencil size={15} />
            </ActionIcon>
          </Tooltip>
          <Tooltip label="Topic löschen">
            <ActionIcon
              color="red"
              size="sm"
              variant="subtle"
              onClick={() => onDelete({ id: topic.entity_id, kind: 'topic', name: topic.name })}
            >
              <IconTrash size={15} />
            </ActionIcon>
          </Tooltip>
        </Group>
      </Group>

      <Collapse expanded={isExpanded}>
        <Group
          className="realization-tree-row realization-document-row"
          gap={6}
          justify="space-between"
          pl={8 + (depth + 1) * 22}
          pr="xs"
          wrap="nowrap"
        >
          <Button
            color="gray"
            leftSection={entry.document === null
              ? <IconPlus size={15} />
              : <IconFileText size={16} />}
            size="compact-sm"
            variant="subtle"
            onClick={() => onOpenDocument(entry)}
          >
            {entry.document === null ? 'Dokument erstellen' : 'Dokument'}
          </Button>
          {entry.document !== null && (
            <Tooltip label="Dokument löschen">
              <ActionIcon
                color="red"
                size="sm"
                variant="subtle"
                onClick={() => onDelete({
                  id: entry.document!.entity_id,
                  kind: 'document',
                  name: `Dokument von ${topic.name}`,
                })}
              >
                <IconTrash size={15} />
              </ActionIcon>
            </Tooltip>
          )}
        </Group>
        {children.map((child) => (
          <TopicTreeItem
            key={child.topic.entity_id}
            childrenByParent={childrenByParent}
            depth={depth + 1}
            entry={child}
            expanded={expanded}
            onCreateChild={onCreateChild}
            onDelete={onDelete}
            onEdit={onEdit}
            onOpenDocument={onOpenDocument}
            onToggle={onToggle}
          />
        ))}
      </Collapse>
    </>
  )
}

export function RealizationView() {
  const graph = useTopicGraph()
  const [areaDialog, setAreaDialog] = useState<DevelopmentArea | null | undefined>()
  const [topicDialog, setTopicDialog] = useState<TopicDialogState | null>(null)
  const [deleteTarget, setDeleteTarget] = useState<DeleteTarget | null>(null)
  const [expanded, setExpanded] = useState(() => new Set<number>())
  const [selectedTopicId, setSelectedTopicId] = useState<number | null>(null)
  const [documentDrafts] = useState(() => new Map<number, string>())
  const entries = useMemo(() => graph.data?.topics ?? [], [graph.data?.topics])
  const selectedEntry = entries.find(({ topic }) => topic.entity_id === selectedTopicId)
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
        key={selectedEntry === undefined ? 'hierarchy' : 'split'}
        defaultLayout={selectedEntry === undefined
          ? { hierarchy: 100 }
          : { editor: 40, hierarchy: 60 }}
        orientation="horizontal"
        style={{ height: '100%', width: '100%' }}
      >
        <Panel id="hierarchy" minSize="30%">
          <Box className="realization-hierarchy-scroll">
            <Box maw={1000} mx="auto" p="md">
              <Group justify="space-between" mb="md">
                <Title order={2}>Verwirklichung</Title>
                <Button leftSection={<IconPlus size={16} />} onClick={() => setAreaDialog(null)}>
                  Bereich
                </Button>
              </Group>

              <Stack gap="md">
                {graph.data.development_areas.map((area) => {
                  const roots = entries.filter(({ topic }) =>
                    topic.parent_topic_entity_id === null &&
                    topic.development_area_id === area.id,
                  )
                  return (
                    <Paper key={area.id} className="realization-area" withBorder>
                      <Box
                        className="realization-area-banner"
                        style={{
                          backgroundColor: area.background_color ?? 'var(--mantine-color-gray-1)',
                          backgroundImage: area.banner_image_path === null
                            ? undefined
                            : `linear-gradient(90deg, rgb(0 0 0 / 45%), rgb(0 0 0 / 10%)), url("${area.banner_image_path}")`,
                          color: area.banner_image_path === null ? undefined : 'white',
                        }}
                      >
                        <Group justify="space-between" wrap="nowrap">
                          <Group gap="sm" miw={0} wrap="nowrap">
                            <ThemeIcon color="gray" size="lg" variant="light">
                              {area.icon === null ? <IconSparkles size={19} /> : (
                                <Text fw={700} lineClamp={1} size="xs">{area.icon}</Text>
                              )}
                            </ThemeIcon>
                            <Title lineClamp={1} order={3}>{area.name}</Title>
                          </Group>
                          <Group gap={4} wrap="nowrap">
                            <Tooltip label="Root-Topic erstellen">
                              <ActionIcon
                                color={area.banner_image_path === null ? 'blue' : 'gray'}
                                variant="light"
                                onClick={() => setTopicDialog({
                                  areaId: area.id,
                                  parentTopicId: null,
                                  topic: null,
                                })}
                              >
                                <IconFolderPlus size={17} />
                              </ActionIcon>
                            </Tooltip>
                            <Tooltip label="Bereich bearbeiten">
                              <ActionIcon
                                color={area.banner_image_path === null ? 'gray' : 'gray'}
                                variant="light"
                                onClick={() => setAreaDialog(area)}
                              >
                                <IconPencil size={17} />
                              </ActionIcon>
                            </Tooltip>
                            <Tooltip label="Bereich löschen">
                              <ActionIcon
                                color="red"
                                variant="light"
                                onClick={() => setDeleteTarget({
                                  id: area.id,
                                  kind: 'area',
                                  name: area.name,
                                })}
                              >
                                <IconTrash size={17} />
                              </ActionIcon>
                            </Tooltip>
                          </Group>
                        </Group>
                      </Box>
                      <Box py={4}>
                        {roots.length === 0 ? (
                          <Text c="dimmed" p="sm" size="sm">Keine Root-Topics.</Text>
                        ) : roots.map((entry) => (
                          <TopicTreeItem
                            key={entry.topic.entity_id}
                            childrenByParent={childrenByParent}
                            depth={0}
                            entry={entry}
                            expanded={expanded}
                            onCreateChild={createChild}
                            onDelete={setDeleteTarget}
                            onEdit={(topic) => setTopicDialog({
                              areaId: null,
                              parentTopicId: null,
                              topic,
                            })}
                            onOpenDocument={(topicEntry) => {
                              setSelectedTopicId(topicEntry.topic.entity_id)
                              setExpanded((current) => new Set(current).add(topicEntry.topic.entity_id))
                            }}
                            onToggle={toggleTopic}
                          />
                        ))}
                      </Box>
                    </Paper>
                  )
                })}
                {unassignedRoots.length > 0 && (
                  <Paper className="realization-area" withBorder>
                    <Box
                      className="realization-area-banner"
                      style={{ backgroundColor: 'var(--mantine-color-gray-1)' }}
                    >
                      <Group gap="sm">
                        <ThemeIcon color="gray" size="lg" variant="light">
                          <IconFolder size={19} />
                        </ThemeIcon>
                        <Title order={3}>Ohne Bereich</Title>
                      </Group>
                    </Box>
                    <Box py={4}>
                      {unassignedRoots.map((entry) => (
                        <TopicTreeItem
                          key={entry.topic.entity_id}
                          childrenByParent={childrenByParent}
                          depth={0}
                          entry={entry}
                          expanded={expanded}
                          onCreateChild={createChild}
                          onDelete={setDeleteTarget}
                          onEdit={(topic) => setTopicDialog({
                            areaId: null,
                            parentTopicId: null,
                            topic,
                          })}
                          onOpenDocument={(topicEntry) => {
                            setSelectedTopicId(topicEntry.topic.entity_id)
                            setExpanded((current) =>
                              new Set(current).add(topicEntry.topic.entity_id),
                            )
                          }}
                          onToggle={toggleTopic}
                        />
                      ))}
                    </Box>
                  </Paper>
                )}
              </Stack>
            </Box>
          </Box>
        </Panel>

        {selectedEntry !== undefined && (
          <>
            <PanelSeparator className="realization-panel-separator" />
            <Panel id="editor" minSize="30%">
              <TopicDocumentPanel
                key={selectedEntry.topic.entity_id}
                document={selectedEntry.document}
                drafts={documentDrafts}
                flashcards={selectedEntry.flashcards}
                topic={selectedEntry.topic}
                onClose={() => setSelectedTopicId(null)}
              />
            </Panel>
          </>
        )}
      </PanelGroup>

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
