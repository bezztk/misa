import { useMutation } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type { CreateTopic, CreateTopicResponse } from './types'

function createTopic(topic: CreateTopic) {
  return apiClient.post<CreateTopicResponse>('/topics', topic)
}

export function useCreateTopic() {
  return useMutation({ mutationFn: createTopic })
}
