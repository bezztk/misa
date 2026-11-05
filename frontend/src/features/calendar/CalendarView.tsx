import { useMemo, useState } from 'react'
import {
  Alert, Box, Button, Center, Checkbox, Divider, Group, Loader, Modal,
  Stack, Text, Textarea, TextInput, Title,
} from '@mantine/core'
import { useForm } from '@mantine/form'
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
import type { CalendarEntry, CreateCalendarEntry } from './types'
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
  planned_start_at: string
  planned_end_at: string
  actual_start_at: string
  actual_end_at: string
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
  const entry = state.mode === 'edit' ? state.entry : null
  const selectedTime = state.mode === 'create' ? state.selectedTime : null
  const createCalendarEntry = useCreateCalendarEntry()
  const updateCalendarEntry = useUpdateCalendarEntry()
  const form = useForm<CalendarEntryFormValues>({
    initialValues: {
      title: entry?.title ?? (state.mode === 'create' ? state.title : ''),
      note: entry?.note ?? '',
      planned_start_at:
        entry === null
          ? selectedTime?.start ?? ''
          : toLocalDateTime(entry.planned_start_at),
      planned_end_at:
        entry === null
          ? selectedTime?.end ?? ''
          : toLocalDateTime(entry.planned_end_at),
      actual_start_at: toLocalDateTime(entry?.actual_start_at ?? null),
      actual_end_at: toLocalDateTime(entry?.actual_end_at ?? null),
      is_reminder: entry?.is_reminder ?? false,
    },
    validate: {
      title: (value) => {
        const length = [...value.trim()].length
        if (length === 0) return 'Titel ist erforderlich.'
        if (length > 150) return 'Der Titel darf maximal 150 Zeichen haben.'
        return null
      },
      planned_start_at: (value) =>
        value === '' ? 'Startzeit ist erforderlich.' : null,
      planned_end_at: (value, values) => {
        if (values.is_reminder) return null
        if (value === '') return 'Endzeit ist erforderlich.'
        if (new Date(value) <= new Date(values.planned_start_at)) {
          return 'Die Endzeit muss nach der Startzeit liegen.'
        }
        return null
      },
      actual_end_at: (value, values) => {
        if (values.is_reminder) return null
        if (value === '') return null
        if (values.actual_start_at === '') {
          return 'Für eine Endzeit ist eine Startzeit erforderlich.'
        }
        if (new Date(value) < new Date(values.actual_start_at)) {
          return 'Die Endzeit darf nicht vor der Startzeit liegen.'
        }
        return null
      },
    },
  })
  const isSaving = createCalendarEntry.isPending || updateCalendarEntry.isPending
  const isError = createCalendarEntry.isError || updateCalendarEntry.isError

  const submit = (values: CalendarEntryFormValues) => {
    const calendarEntry: CreateCalendarEntry = {
      source_entity_id:
        entry?.source_entity_id ??
        (state.mode === 'create' ? state.sourceEntityId : null),
      title: values.title.trim(),
      note: values.note.trim() || null,
      is_reminder: values.is_reminder,
      planned_start_at: new Date(values.planned_start_at).toISOString(),
      planned_end_at: values.is_reminder
        ? null
        : new Date(values.planned_end_at).toISOString(),
      actual_start_at: !values.is_reminder && values.actual_start_at
        ? new Date(values.actual_start_at).toISOString()
        : null,
      actual_end_at: !values.is_reminder && values.actual_end_at
        ? new Date(values.actual_end_at).toISOString()
        : null,
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
          <TextInput data-autofocus required label="Titel" {...form.getInputProps('title')} />
          <Textarea autosize label="Notiz" minRows={3} {...form.getInputProps('note')} />
          <TextInput
            required label="Geplanter Start" type="datetime-local"
            {...form.getInputProps('planned_start_at')}
          />
          {!form.values.is_reminder && (
            <TextInput
              required label="Geplantes Ende" type="datetime-local"
              {...form.getInputProps('planned_end_at')}
            />
          )}
          <Checkbox
            label="Erinnerung"
            {...form.getInputProps('is_reminder', { type: 'checkbox' })}
          />
          {!form.values.is_reminder && (
            <>
              <TextInput
                label="Tatsächlicher Start" type="datetime-local"
                {...form.getInputProps('actual_start_at')}
              />
              <TextInput
                label="Tatsächliches Ende" type="datetime-local"
                {...form.getInputProps('actual_end_at')}
              />
            </>
          )}
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
              <Button loading={isSaving} type="submit">Speichern</Button>
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
  const [formState, setFormState] = useState<CalendarFormState | null>(null)
  const [entryToDelete, setEntryToDelete] = useState<CalendarEntry | null>(null)
  const events = useMemo(
    () =>
      (calendarEntries.data ?? []).map((entry) => {
        let start = entry.planned_start_at
        let end = entry.planned_end_at ?? undefined

        if (!entry.is_reminder && entry.actual_start_at !== null) {
          start = entry.actual_start_at

          if (entry.actual_end_at !== null) {
            end = entry.actual_end_at
          } else if (entry.planned_end_at !== null) {
            const plannedDuration =
              new Date(entry.planned_end_at).getTime() -
              new Date(entry.planned_start_at).getTime()
            end = new Date(
              new Date(entry.actual_start_at).getTime() + plannedDuration,
            ).toISOString()
          }
        }

        return {
          id: String(entry.entity_id),
          title: entry.title,
          start,
          end: entry.is_reminder ? undefined : end,
        }
      }),
    [calendarEntries.data],
  )

  const openCreateForm = (selection: DateSelectInfo) => {
    const start = new Date(selection.start)
    let end = new Date(selection.end)

    if (selection.allDay) {
      start.setHours(9, 0, 0, 0)
      end = new Date(start.getTime() + 60 * 60 * 1000)
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
