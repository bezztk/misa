import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type {
  CalendarEntry,
  CreateCalendarEntry,
  CreateCalendarEntryResponse,
  UpdateCalendarEntry,
} from './types'

const calendarKeys = {
  list: ['calendar', 'list'] as const,
  detail: (entityId: number) => ['calendar', 'detail', entityId] as const,
}

export function getCalendarEntries() {
  return apiClient.get<CalendarEntry[]>('/calendar')
}

export function getCalendarEntry(entityId: number) {
  return apiClient.get<CalendarEntry>(`/calendar/${entityId}`)
}

export function createCalendarEntry(entry: CreateCalendarEntry) {
  return apiClient.post<CreateCalendarEntryResponse>('/calendar', entry)
}

export function updateCalendarEntry(
  entityId: number,
  entry: UpdateCalendarEntry,
) {
  return apiClient.put<void>(`/calendar/${entityId}`, entry)
}

export function deleteCalendarEntry(entityId: number) {
  return apiClient.delete(`/entities/${entityId}`)
}

export function useCalendarEntries() {
  return useQuery({
    queryKey: calendarKeys.list,
    queryFn: getCalendarEntries,
  })
}

export function useCalendarEntry(entityId: number) {
  return useQuery({
    queryKey: calendarKeys.detail(entityId),
    queryFn: () => getCalendarEntry(entityId),
  })
}

export function useCreateCalendarEntry() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: createCalendarEntry,
    onSuccess: () =>
      queryClient.invalidateQueries({ queryKey: calendarKeys.list }),
  })
}

export function useUpdateCalendarEntry() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({
      entityId,
      entry,
    }: {
      entityId: number
      entry: UpdateCalendarEntry
    }) => updateCalendarEntry(entityId, entry),
    onSuccess: async (_, { entityId }) => {
      await Promise.all([
        queryClient.invalidateQueries({ queryKey: calendarKeys.list }),
        queryClient.invalidateQueries({
          queryKey: calendarKeys.detail(entityId),
        }),
      ])
    },
  })
}

export function useDeleteCalendarEntry() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteCalendarEntry,
    onSuccess: async (_, entityId) => {
      queryClient.removeQueries({ queryKey: calendarKeys.detail(entityId) })
      await queryClient.invalidateQueries({ queryKey: calendarKeys.list })
    },
  })
}
