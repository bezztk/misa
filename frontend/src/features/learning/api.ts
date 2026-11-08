import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query'
import { apiClient } from '../../api/client'
import type { Flashcard } from '../realization/types'
import type { FlashcardPreview, FlashcardReview } from './types'

const dueFlashcardsKey = ['flashcards', 'due'] as const
const flashcardPreviewKey = (id: number) => ['flashcards', id, 'preview'] as const

function getDueFlashcards() {
  return apiClient.get<Flashcard[]>('/flashcards/due')
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
    onSuccess: (_, review) => {
      queryClient.setQueryData<Flashcard[]>(dueFlashcardsKey, (flashcards) =>
        flashcards?.filter((flashcard) => flashcard.id !== review.id),
      )
      queryClient.removeQueries({ queryKey: flashcardPreviewKey(review.id) })
    },
  })
}
