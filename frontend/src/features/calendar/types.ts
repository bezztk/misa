export interface CalendarEntry {
  entity_id: number
  created_at: string
  updated_at: string
  version: number
  source_entity_id: number | null
  title: string
  note: string | null
  is_reminder: boolean
  start_at: string
  end_at: string | null
}

export interface CreateCalendarEntry {
  source_entity_id: number | null
  title: string
  note: string | null
  is_reminder: boolean
  start_at: string
  end_at: string | null
}

export type UpdateCalendarEntry = CreateCalendarEntry

export interface CreateCalendarEntryResponse {
  entity_id: number
}
