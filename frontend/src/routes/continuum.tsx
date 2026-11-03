import { Title } from '@mantine/core'
import { createFileRoute } from '@tanstack/react-router'

export const Route = createFileRoute('/continuum')({
  component: () => <Title>Continuum</Title>,
})
