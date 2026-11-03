import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type {
  CreateDevelopmentArea,
  CreateDevelopmentAreaResponse,
  DevelopmentArea,
  UpdateDevelopmentArea,
} from './types'

const developmentAreasKey = ['development-areas'] as const

export function getDevelopmentAreas() {
  return apiClient.get<DevelopmentArea[]>('/development-areas')
}

export function createDevelopmentArea(area: CreateDevelopmentArea) {
  return apiClient.post<CreateDevelopmentAreaResponse>(
    '/development-areas',
    area,
  )
}

export function updateDevelopmentArea(
  id: number,
  area: UpdateDevelopmentArea,
) {
  return apiClient.patch<void>(`/development-areas/${id}`, area)
}

export function deleteDevelopmentArea(id: number) {
  return apiClient.delete(`/development-areas/${id}`)
}

export function useDevelopmentAreas(enabled = true) {
  return useQuery({
    queryKey: developmentAreasKey,
    queryFn: getDevelopmentAreas,
    enabled,
  })
}

export function useCreateDevelopmentArea() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: createDevelopmentArea,
    onSuccess: () =>
      queryClient.invalidateQueries({ queryKey: developmentAreasKey }),
  })
}

export function useUpdateDevelopmentArea() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({
      id,
      area,
    }: {
      id: number
      area: UpdateDevelopmentArea
    }) => updateDevelopmentArea(id, area),
    onSuccess: () =>
      queryClient.invalidateQueries({ queryKey: developmentAreasKey }),
  })
}

export function useDeleteDevelopmentArea() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteDevelopmentArea,
    onSuccess: () =>
      queryClient.invalidateQueries({ queryKey: developmentAreasKey }),
  })
}
