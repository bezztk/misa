import { createFileRoute } from '@tanstack/react-router'
import { RealizationView } from '../features/realization/HierarchicalRealizationView'

export const Route = createFileRoute('/realization')({
  component: RealizationView,
})
