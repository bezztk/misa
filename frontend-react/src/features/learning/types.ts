import type { Flashcard } from '../realization/types'

export interface FlashcardPreview {
  again: number
  hard: number
  good: number
  easy: number
}

export interface FlashcardReview {
  id: number
  rating: number
  duration_ms: number
}

export interface FlashcardQueue {
  due: Flashcard[]
  upcoming: Flashcard[]
}
