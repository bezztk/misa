import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type {
  CreateTopic,
  CreateTopicResponse,
  TopicGraphEntry,
} from './types'

export const topicGraphKey = ['topic', 'graph'] as const

function getTopicGraph() {
  return apiClient.get<TopicGraphEntry[]>('/topic/graph')
}

export function useTopicGraph() {
  return useQuery({
    queryKey: topicGraphKey,
    queryFn: getTopicGraph,
    refetchOnMount: 'always',
  })
}

function createTopic(topic: CreateTopic) {
  return apiClient.post<CreateTopicResponse>('/topics', topic)
}

export function useCreateTopic() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: createTopic,
    onSuccess: () => queryClient.invalidateQueries({ queryKey: topicGraphKey }),
  })
}
