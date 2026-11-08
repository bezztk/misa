import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type {
  CreateTopic,
  CreateTopicResponse,
  CreateFlashcard,
  CreateFlashcardResponse,
  Document,
  TopicGraphEntry,
  UpdateFlashcard,
} from './types'

export const topicGraphKey = ['topic', 'graph'] as const
const topicDocumentKey = (topicId: number) => ['topic', topicId, 'document'] as const

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

function getDocument(entityId: number) {
  return apiClient.get<Document>(`/documents/${entityId}`)
}

function createDocumentForTopic(topicId: number) {
  return apiClient.post<Document>(`/topics/${topicId}/document`, {})
}

export function useTopicDocument(
  topicId: number,
  documentEntityId: number | null,
) {
  return useQuery({
    queryKey: topicDocumentKey(topicId),
    queryFn: () =>
      documentEntityId === null
        ? createDocumentForTopic(topicId)
        : getDocument(documentEntityId),
    refetchOnMount: 'always',
  })
}

export function updateDocumentContent(entityId: number, content: string) {
  return apiClient.patch<void>(`/documents/${entityId}`, { content })
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

function createFlashcard(flashcard: CreateFlashcard) {
  return apiClient.post<CreateFlashcardResponse>('/flashcards', flashcard)
}

export function useCreateFlashcard() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: createFlashcard,
    onSuccess: () => queryClient.invalidateQueries({ queryKey: topicGraphKey }),
  })
}

function updateFlashcard(id: number, flashcard: UpdateFlashcard) {
  return apiClient.patch<void>(`/flashcards/${id}`, flashcard)
}

export function useUpdateFlashcard() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({ id, flashcard }: { id: number; flashcard: UpdateFlashcard }) =>
      updateFlashcard(id, flashcard),
    onSuccess: () => queryClient.invalidateQueries({ queryKey: topicGraphKey }),
  })
}

function deleteFlashcard(id: number) {
  return apiClient.delete(`/flashcards/${id}`)
}

export function useDeleteFlashcard() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteFlashcard,
    onSuccess: () => queryClient.invalidateQueries({ queryKey: topicGraphKey }),
  })
}
