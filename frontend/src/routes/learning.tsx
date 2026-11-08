import { createFileRoute } from '@tanstack/react-router'
import { LearningView } from '../features/learning/LearningView'

export const Route = createFileRoute('/learning')({
  component: LearningView,
})
