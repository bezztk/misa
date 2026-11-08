import { createFileRoute } from '@tanstack/react-router'
import { RealizationView } from '../features/realization/RealizationView'

export const Route = createFileRoute('/realization')({
  component: RealizationView,
})
