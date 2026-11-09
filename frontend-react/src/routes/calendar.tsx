import { createFileRoute } from '@tanstack/react-router'
import { CalendarView } from '../features/calendar/CalendarView'

export const Route = createFileRoute('/calendar')({
  component: CalendarView,
})
