import { Button, Container, Group } from '@mantine/core'
import { createRootRoute, Link, Outlet } from '@tanstack/react-router'

export const Route = createRootRoute({
  component: () => (
    <>
      <Group component="nav" p="md">
        <Button component={Link} to="/tasks" variant="subtle">
          Aufgaben
        </Button>
        <Button component={Link} to="/continuum" variant="subtle">
          Continuum
        </Button>
      </Group>
      <Container fluid>
        <Outlet />
      </Container>
    </>
  ),
})
