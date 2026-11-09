import { queryOptions, useQuery } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type { Metadata } from './types'

export const metadataKey = ['metadata'] as const

export function getMetadata() {
  return apiClient.get<Metadata>('/metadata')
}

export const metadataQueryOptions = queryOptions({
  queryKey: metadataKey,
  queryFn: getMetadata,
  staleTime: Infinity,
})

export function useMetadata() {
  return useQuery(metadataQueryOptions)
}
