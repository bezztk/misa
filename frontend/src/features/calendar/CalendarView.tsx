import { useMemo, useState } from 'react'
import {
  Alert, Box, Button, Center, Checkbox, Divider, Group, Loader, Modal,
  Stack, Text, Textarea, TextInput, Title,
} from '@mantine/core'
import { useForm } from '@mantine/form'
import { IconListCheck } from '@tabler/icons-react'
import { useQueries } from '@tanstack/react-query'
import { useNavigate } from '@tanstack/react-router'
import FullCalendar, { type DateSelectInfo, type EventClickInfo } from '@fullcalendar/react'
import dayGridPlugin from '@fullcalendar/react/daygrid'
import interactionPlugin from '@fullcalendar/react/interaction'
import deLocale from '@fullcalendar/react/locales/de'
import classicThemePlugin from '@fullcalendar/react/themes/classic'
import timeGridPlugin from '@fullcalendar/react/timegrid'
import {
  useCalendarEntries, useCreateCalendarEntry, useDeleteCalendarEntry,
  useUpdateCalendarEntry,
} from './api'
import { getTask, taskKeys, useTask } from '../tasks/api'
import type { CalendarEntry, CreateCalendarEntry } from './types'
import type { Task } from '../tasks/types'
import '@fullcalendar/react/skeleton.css'
import '@fullcalendar/react/themes/classic/theme.css'
import '@fullcalendar/react/themes/classic/palette.css'
import './CalendarView.css'

interface SelectedTime {
  start: string
  end: string
}

export type CalendarFormState =
  | {
      mode: 'create'
      selectedTime: SelectedTime
      sourceEntityId: number | null
      title: string
    }
  | { mode: 'edit'; entry: CalendarEntry }

interface CalendarEntryFormValues {
  title: string
  note: string
  start_at: string
  end_at: string
  is_reminder: boolean
}

function toLocalDateTime(value: string | Date | null) {
  if (value === null) return ''

  const date = new Date(value)
  const offset = date.getTimezoneOffset() * 60_000
  return new Date(date.getTime() - offset).toISOString().slice(0, 16)
}

export function CalendarEntryModal({
  state,
  onClose,
  onDelete,
}: {
  state: CalendarFormState
  onClose: () => void
  onDelete?: (entry: CalendarEntry) => void
}) {
  const navigate = useNavigate()
  const entry = state.mode === 'edit' ? state.entry : null
  const selectedTime = state.mode === 'create' ? state.selectedTime : null
  const sourceEntityId =
    entry?.source_entity_id ??
    (state.mode === 'create' ? state.sourceEntityId : null)
  const linkedTask = useTask(sourceEntityId ?? 0, sourceEntityId !== null)
  const createCalendarEntry = useCreateCalendarEntry()
  const updateCalendarEntry = useUpdateCalendarEntry()
  const [defaultTime] = useState(() => {
    const start = new Date()
    const end = new Date(start.getTime() + 30 * 60 * 1000)
    return { start: toLocalDateTime(start), end: toLocalDateTime(end) }
  })
  const form = useForm<CalendarEntryFormValues>({
    initialValues: {
      title: entry?.title ?? (state.mode === 'create' ? state.title : ''),
      note: entry?.note ?? '',
      start_at:
        entry === null
          ? selectedTime?.start || defaultTime.start
          : toLocalDateTime(entry.start_at),
      end_at:
        entry === null
          ? selectedTime?.end || defaultTime.end
          : toLocalDateTime(entry.end_at),
      is_reminder: entry?.is_reminder ?? false,
    },
    validate: {
      title: (value) => {
        if (sourceEntityId !== null) return null
        const length = [...value.trim()].length
        if (length === 0) return 'Titel ist erforderlich.'
        if (length > 150) return 'Der Titel darf maximal 150 Zeichen haben.'
        return null
      },
      start_at: (value) =>
        value === '' ? 'Startzeit ist erforderlich.' : null,
      end_at: (value, values) => {
        if (values.is_reminder) return null
        if (value === '') return 'Endzeit ist erforderlich.'
        if (new Date(value) <= new Date(values.start_at)) {
          return 'Die Endzeit muss nach der Startzeit liegen.'
        }
        return null
      },
    },
  })
  const isSaving = createCalendarEntry.isPending || updateCalendarEntry.isPending
  const isError = createCalendarEntry.isError || updateCalendarEntry.isError

  const submit = (values: CalendarEntryFormValues) => {
    const calendarEntry: CreateCalendarEntry = {
      source_entity_id: sourceEntityId,
      title: linkedTask.data?.name ?? values.title.trim(),
      note: values.note.trim() || null,
      is_reminder: values.is_reminder,
      start_at: new Date(values.start_at).toISOString(),
      end_at: values.is_reminder
        ? null
        : new Date(values.end_at).toISOString(),
    }

    if (entry === null) {
      createCalendarEntry.mutate(calendarEntry, { onSuccess: onClose })
      return
    }

    updateCalendarEntry.mutate(
      { entityId: entry.entity_id, entry: calendarEntry },
      { onSuccess: onClose },
    )
  }

  return (
    <Modal
      centered
      opened
      onClose={onClose}
      title={entry === null ? 'Kalendereintrag erstellen' : 'Kalendereintrag bearbeiten'}
    >
      <Box component="form" onSubmit={form.onSubmit(submit)}>
        <Stack>
          {sourceEntityId !== null ? (
            <>
              <Group align="flex-end" wrap="nowrap">
                <TextInput
                  label="Titel"
                  readOnly
                  style={{ flex: 1 }}
                  value={
                    linkedTask.data?.name ??
                    (state.mode === 'create' ? state.title : entry?.title ?? '')
                  }
                />
                <Button
                  type="button"
                  variant="light"
                  onClick={() =>
                    void navigate({
                      to: '/tasks',
                      search: { taskId: sourceEntityId },
                    })
                  }
                >
                  Aufgabe öffnen
                </Button>
              </Group>
              <Textarea
                autosize
                label="Aufgabenbeschreibung"
                minRows={2}
                readOnly
                value={linkedTask.data?.description ?? ''}
              />
              {linkedTask.isError && (
                <Alert color="red" title="Fehler">
                  Die verknüpfte Aufgabe konnte nicht geladen werden.
                </Alert>
              )}
            </>
          ) : (
            <TextInput
              data-autofocus
              required
              label="Titel"
              {...form.getInputProps('title')}
            />
          )}
          <Textarea autosize label="Notiz" minRows={3} {...form.getInputProps('note')} />
          <TextInput
            required label="Startzeit" type="datetime-local"
            {...form.getInputProps('start_at')}
          />
          {!form.values.is_reminder && (
            <TextInput
              required label="Endzeit" type="datetime-local"
              {...form.getInputProps('end_at')}
            />
          )}
          <Checkbox
            label="Erinnerung"
            {...form.getInputProps('is_reminder', { type: 'checkbox' })}
          />
          {isError && (
            <Alert color="red" title="Fehler">
              Der Kalendereintrag konnte nicht gespeichert werden.
            </Alert>
          )}
          <Group justify={entry === null ? 'flex-end' : 'space-between'}>
            {entry !== null && onDelete !== undefined && (
              <Button
                color="red" disabled={isSaving} type="button" variant="light"
                onClick={() => onDelete(entry)}
              >
                Löschen
              </Button>
            )}
            <Group>
              <Button disabled={isSaving} type="button" variant="default" onClick={onClose}>
                Abbrechen
              </Button>
              <Button
                disabled={sourceEntityId !== null && linkedTask.data === undefined}
                loading={isSaving}
                type="submit"
              >
                Speichern
              </Button>
            </Group>
          </Group>
        </Stack>
      </Box>
    </Modal>
  )
}

function DeleteCalendarEntryModal({
  entry,
  onClose,
}: {
  entry: CalendarEntry
  onClose: () => void
}) {
  const deleteCalendarEntry = useDeleteCalendarEntry()

  return (
    <Modal centered opened onClose={onClose} title="Kalendereintrag löschen">
      <Stack>
        <Text>Möchtest du „{entry.title}“ wirklich löschen?</Text>
        {deleteCalendarEntry.isError && (
          <Alert color="red" title="Fehler">
            Der Kalendereintrag konnte nicht gelöscht werden.
          </Alert>
        )}
        <Group justify="flex-end">
          <Button
            disabled={deleteCalendarEntry.isPending} type="button"
            variant="default" onClick={onClose}
          >
            Abbrechen
          </Button>
          <Button
            color="red" loading={deleteCalendarEntry.isPending}
            onClick={() =>
              deleteCalendarEntry.mutate(entry.entity_id, { onSuccess: onClose })
            }
          >
            Löschen
          </Button>
        </Group>
      </Stack>
    </Modal>
  )
}

export function CalendarView() {
  const calendarEntries = useCalendarEntries()
  const taskIds = useMemo(
    () =>
      [...new Set(
        (calendarEntries.data ?? []).flatMap((entry) =>
          entry.source_entity_id === null ? [] : [entry.source_entity_id],
        ),
      )],
    [calendarEntries.data],
  )
  const taskQueries = useQueries({
    queries: taskIds.map((entityId) => ({
      queryKey: taskKeys.detail(entityId),
      queryFn: () => getTask(entityId),
    })),
  })
  const tasksById = new Map<number, Task>()
  taskQueries.forEach((query, index) => {
    if (query.data !== undefined) tasksById.set(taskIds[index], query.data)
  })
  const [formState, setFormState] = useState<CalendarFormState | null>(null)
  const [entryToDelete, setEntryToDelete] = useState<CalendarEntry | null>(null)
  const events = (calendarEntries.data ?? []).map((entry) => {
    const task =
      entry.source_entity_id === null
        ? undefined
        : tasksById.get(entry.source_entity_id)

    return {
      id: String(entry.entity_id),
      title: task?.name ?? entry.title,
      start: entry.start_at,
      end: entry.is_reminder ? undefined : (entry.end_at ?? undefined),
      extendedProps: { taskId: task?.entity_id },
    }
  })

  const openCreateForm = (selection: DateSelectInfo) => {
    const start = new Date(selection.start)
    let end = new Date(selection.end)

    if (selection.allDay) {
      start.setHours(9, 0, 0, 0)
      end = new Date(start.getTime() + 30 * 60 * 1000)
    }

    setFormState({
      mode: 'create',
      sourceEntityId: null,
      title: '',
      selectedTime: { start: toLocalDateTime(start), end: toLocalDateTime(end) },
    })
  }

  const openEditForm = (event: EventClickInfo) => {
    const entry = calendarEntries.data?.find(
      (candidate) => candidate.entity_id === Number(event.event.id),
    )
    if (entry !== undefined) setFormState({ mode: 'edit', entry })
  }

  return (
    <Stack gap="md">
      <Box>
        <Title>Kalender</Title>
        <Divider mt="md" />
      </Box>
      {calendarEntries.isPending ? (
        <Center><Loader /></Center>
      ) : calendarEntries.isError ? (
        <Alert color="red" title="Fehler">
          Die Kalendereinträge konnten nicht geladen werden.
        </Alert>
      ) : (
        <Box className="calendar-view">
          <FullCalendar
            allDaySlot={false}
            eventClick={openEditForm}
            eventContent={(event) => (
              <Group gap={4} wrap="nowrap">
                {event.event.extendedProps.taskId !== undefined && (
                  <IconListCheck size={12} />
                )}
                {event.timeText && <Text size="xs">{event.timeText}</Text>}
                <Text size="xs" truncate>{event.event.title}</Text>
              </Group>
            )}
            events={events}
            firstDay={1}
            headerToolbar={{
              left: 'prev,next today', center: 'title',
              right: 'dayGridMonth,timeGridWeek,timeGridDay',
            }}
            initialView="dayGridMonth"
            locale="de"
            locales={[deLocale]}
            nowIndicator
            plugins={[
              classicThemePlugin, dayGridPlugin, timeGridPlugin, interactionPlugin,
            ]}
            selectable
            select={openCreateForm}
            selectMirror
          />
        </Box>
      )}
      {formState !== null && (
        <CalendarEntryModal
          state={formState}
          onClose={() => setFormState(null)}
          onDelete={(entry) => {
            setFormState(null)
            setEntryToDelete(entry)
          }}
        />
      )}
      {entryToDelete !== null && (
        <DeleteCalendarEntryModal
          entry={entryToDelete}
          onClose={() => setEntryToDelete(null)}
        />
      )}
    </Stack>
  )
}
