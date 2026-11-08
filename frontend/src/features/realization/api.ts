import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type {
  CreateTopic,
  CreateTopicResponse,
  CreateDevelopmentArea,
  CreateDevelopmentAreaResponse,
  CreateFlashcard,
  CreateFlashcardResponse,
  Document,
  TopicGraph,
  UpdateDevelopmentArea,
  UpdateFlashcard,
  UpdateTopic,
} from './types'

export const topicGraphKey = ['topic', 'graph'] as const
const topicDocumentKey = (topicId: number) => ['topic', topicId, 'document'] as const

function getTopicGraph() {
  return apiClient.get<TopicGraph>('/topic/graph')
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

function updateTopic(id: number, topic: UpdateTopic) {
  return apiClient.put<void>(`/topics/${id}`, topic)
}

export function useUpdateTopic() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({ id, topic }: { id: number; topic: UpdateTopic }) =>
      updateTopic(id, topic),
    onSuccess: () => queryClient.invalidateQueries({ queryKey: topicGraphKey }),
  })
}

function deleteEntity(id: number) {
  return apiClient.delete(`/entities/${id}`)
}

export function useDeleteEntity() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteEntity,
    onSuccess: () => queryClient.invalidateQueries({ queryKey: topicGraphKey }),
  })
}

function createDevelopmentArea(area: CreateDevelopmentArea) {
  return apiClient.post<CreateDevelopmentAreaResponse>('/development-areas', area)
}

export function useCreateDevelopmentArea() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: createDevelopmentArea,
    onSuccess: () => queryClient.invalidateQueries({ queryKey: topicGraphKey }),
  })
}

function updateDevelopmentArea(id: number, area: UpdateDevelopmentArea) {
  return apiClient.patch<void>(`/development-areas/${id}`, area)
}

export function useUpdateDevelopmentArea() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: ({ area, id }: { area: UpdateDevelopmentArea; id: number }) =>
      updateDevelopmentArea(id, area),
    onSuccess: () => queryClient.invalidateQueries({ queryKey: topicGraphKey }),
  })
}

function deleteDevelopmentArea(id: number) {
  return apiClient.delete(`/development-areas/${id}`)
}

export function useDeleteDevelopmentArea() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: deleteDevelopmentArea,
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
