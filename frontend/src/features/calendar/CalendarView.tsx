import { useMemo, useState } from 'react'
import {
  Alert,
  Box,
  Button,
  Center,
  Checkbox,
  Divider,
  Group,
  Loader,
  Modal,
  Stack,
  Textarea,
  TextInput,
  Title,
} from '@mantine/core'
import { useForm } from '@mantine/form'
import FullCalendar, { type DateSelectInfo } from '@fullcalendar/react'
import dayGridPlugin from '@fullcalendar/react/daygrid'
import interactionPlugin from '@fullcalendar/react/interaction'
import deLocale from '@fullcalendar/react/locales/de'
import classicThemePlugin from '@fullcalendar/react/themes/classic'
import timeGridPlugin from '@fullcalendar/react/timegrid'
import { useCalendarEntries, useCreateCalendarEntry } from './api'
import '@fullcalendar/react/skeleton.css'
import '@fullcalendar/react/themes/classic/theme.css'
import '@fullcalendar/react/themes/classic/palette.css'
import './CalendarView.css'

interface SelectedTime {
  start: string
  end: string
}

interface CalendarEntryFormValues {
  title: string
  note: string
  planned_start_at: string
  planned_end_at: string
  is_reminder: boolean
}

function toLocalDateTime(date: Date) {
  const offset = date.getTimezoneOffset() * 60_000
  return new Date(date.getTime() - offset).toISOString().slice(0, 16)
}

function CalendarEntryModal({
  selectedTime,
  onClose,
}: {
  selectedTime: SelectedTime
  onClose: () => void
}) {
  const createCalendarEntry = useCreateCalendarEntry()
  const form = useForm<CalendarEntryFormValues>({
    initialValues: {
      title: '',
      note: '',
      planned_start_at: selectedTime.start,
      planned_end_at: selectedTime.end,
      is_reminder: false,
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
    },
  })

  const submit = (values: CalendarEntryFormValues) => {
    createCalendarEntry.mutate(
      {
        source_entity_id: null,
        title: values.title.trim(),
        note: values.note.trim() || null,
        is_reminder: values.is_reminder,
        planned_start_at: new Date(values.planned_start_at).toISOString(),
        planned_end_at: values.is_reminder
          ? null
          : new Date(values.planned_end_at).toISOString(),
        actual_start_at: null,
        actual_end_at: null,
      },
      { onSuccess: onClose },
    )
  }

  return (
    <Modal centered opened onClose={onClose} title="Kalendereintrag erstellen">
      <Box component="form" onSubmit={form.onSubmit(submit)}>
        <Stack>
          <TextInput
            data-autofocus
            required
            label="Titel"
            {...form.getInputProps('title')}
          />
          <Textarea
            autosize
            label="Notiz"
            minRows={3}
            {...form.getInputProps('note')}
          />
          <TextInput
            required
            label="Startzeit"
            type="datetime-local"
            {...form.getInputProps('planned_start_at')}
          />
          {!form.values.is_reminder && (
            <TextInput
              required
              label="Endzeit"
              type="datetime-local"
              {...form.getInputProps('planned_end_at')}
            />
          )}
          <Checkbox
            label="Erinnerung"
            {...form.getInputProps('is_reminder', { type: 'checkbox' })}
          />
          {createCalendarEntry.isError && (
            <Alert color="red" title="Fehler">
              Der Kalendereintrag konnte nicht gespeichert werden.
            </Alert>
          )}
          <Group justify="flex-end">
            <Button
              disabled={createCalendarEntry.isPending}
              type="button"
              variant="default"
              onClick={onClose}
            >
              Abbrechen
            </Button>
            <Button loading={createCalendarEntry.isPending} type="submit">
              Speichern
            </Button>
          </Group>
        </Stack>
      </Box>
    </Modal>
  )
}

export function CalendarView() {
  const calendarEntries = useCalendarEntries()
  const [selectedTime, setSelectedTime] = useState<SelectedTime | null>(null)
  const events = useMemo(
    () =>
      (calendarEntries.data ?? []).map((entry) => ({
        id: String(entry.entity_id),
        title: entry.title,
        start: entry.planned_start_at,
        end: entry.is_reminder ? undefined : (entry.planned_end_at ?? undefined),
      })),
    [calendarEntries.data],
  )
  const openForm = (selection: DateSelectInfo) => {
    const start = new Date(selection.start)
    let end = new Date(selection.end)

    if (selection.allDay) {
      start.setHours(9, 0, 0, 0)
      end = new Date(start.getTime() + 60 * 60 * 1000)
    }

    setSelectedTime({
      start: toLocalDateTime(start),
      end: toLocalDateTime(end),
    })
  }

  return (
    <Stack gap="md">
      <Box>
        <Title>Kalender</Title>
        <Divider mt="md" />
      </Box>

      {calendarEntries.isPending ? (
        <Center>
          <Loader />
        </Center>
      ) : calendarEntries.isError ? (
        <Alert color="red" title="Fehler">
          Die Kalendereinträge konnten nicht geladen werden.
        </Alert>
      ) : (
        <Box className="calendar-view">
          <FullCalendar
            allDaySlot={false}
            events={events}
            firstDay={1}
            headerToolbar={{
              left: 'prev,next today',
              center: 'title',
              right: 'dayGridMonth,timeGridWeek,timeGridDay',
            }}
            initialView="dayGridMonth"
            locale="de"
            locales={[deLocale]}
            nowIndicator
            plugins={[
              classicThemePlugin,
              dayGridPlugin,
              timeGridPlugin,
              interactionPlugin,
            ]}
            selectable
            select={openForm}
            selectMirror
          />
        </Box>
      )}
      {selectedTime !== null && (
        <CalendarEntryModal
          selectedTime={selectedTime}
          onClose={() => setSelectedTime(null)}
        />
      )}
    </Stack>
  )
}
