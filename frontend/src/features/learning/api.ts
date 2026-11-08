import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type { FlashcardPreview, FlashcardQueue, FlashcardReview } from './types'

const dueFlashcardsKey = ['flashcards', 'due'] as const
const flashcardPreviewKey = (id: number) => ['flashcards', id, 'preview'] as const

function getDueFlashcards() {
  return apiClient.get<FlashcardQueue>('/flashcards/due')
}

export function useDueFlashcards() {
  return useQuery({
    queryKey: dueFlashcardsKey,
    queryFn: getDueFlashcards,
    refetchOnMount: 'always',
  })
}

function getFlashcardPreview(id: number) {
  return apiClient.get<FlashcardPreview>(`/flashcards/${id}/preview`)
}

export function useFlashcardPreview(id: number, enabled: boolean) {
  return useQuery({
    queryKey: flashcardPreviewKey(id),
    queryFn: () => getFlashcardPreview(id),
    enabled,
  })
}

function reviewFlashcard({ id, ...review }: FlashcardReview) {
  return apiClient.post<void>(`/flashcards/${id}/reviews`, review)
}

export function useReviewFlashcard() {
  const queryClient = useQueryClient()

  return useMutation({
    mutationFn: reviewFlashcard,
    onSuccess: async (_, review) => {
      queryClient.removeQueries({ queryKey: flashcardPreviewKey(review.id) })
      await queryClient.invalidateQueries({ queryKey: dueFlashcardsKey })
    },
  })
}
