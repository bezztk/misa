import { createFileRoute } from '@tanstack/react-router'
import { ContinuumFlow } from '../features/continuum/ContinuumFlow'

export const Route = createFileRoute('/continuum')({
  component: ContinuumFlow,
})
