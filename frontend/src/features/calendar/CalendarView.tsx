import { useMemo } from 'react'
import { Alert, Box, Center, Divider, Loader, Stack, Title } from '@mantine/core'
import FullCalendar from '@fullcalendar/react'
import dayGridPlugin from '@fullcalendar/daygrid'
import timeGridPlugin from '@fullcalendar/timegrid'
import { useCalendarEntries } from './api'
import './CalendarView.css'

export function CalendarView() {
  const calendarEntries = useCalendarEntries()
  const events = useMemo(
    () =>
      (calendarEntries.data ?? []).map((entry) => ({
        id: String(entry.entity_id),
        title: entry.is_reminder ? 'Erinnerung' : 'Termin',
        start: entry.planned_start_at,
        end: entry.is_reminder ? undefined : (entry.planned_end_at ?? undefined),
      })),
    [calendarEntries.data],
  )

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
            buttonText={{
              today: 'Heute',
              month: 'Monat',
              week: 'Woche',
              day: 'Tag',
            }}
            events={events}
            firstDay={1}
            headerToolbar={{
              left: 'prev,next today',
              center: 'title',
              right: 'dayGridMonth,timeGridWeek,timeGridDay',
            }}
            initialView="dayGridMonth"
            locale="de"
            nowIndicator
            plugins={[dayGridPlugin, timeGridPlugin]}
          />
        </Box>
      )}
    </Stack>
  )
}
