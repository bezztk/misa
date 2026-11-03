import { useState } from 'react'
import {
  ActionIcon,
  AppShell,
  Burger,
  Group,
  NavLink,
  Stack,
  Text,
  Title,
} from '@mantine/core'
import { Link, Outlet, useRouterState } from '@tanstack/react-router'

export function AppLayout() {
  const [mobileOpened, setMobileOpened] = useState(false)
  const [navbarCollapsed, setNavbarCollapsed] = useState(false)
  const pathname = useRouterState({
    select: (state) => state.location.pathname,
  })
  const tasksLabel = navbarCollapsed ? (
    <>
      <Text component="span" hiddenFrom="sm">
        Aufgaben
      </Text>
      <Text component="span" visibleFrom="sm">
        A
      </Text>
    </>
  ) : (
    'Aufgaben'
  )
  const continuumLabel = navbarCollapsed ? (
    <>
      <Text component="span" hiddenFrom="sm">
        Continuum
      </Text>
      <Text component="span" visibleFrom="sm">
        C
      </Text>
    </>
  ) : (
    'Continuum'
  )

  return (
    <AppShell
      header={{ height: { base: 56, sm: 0 } }}
      navbar={{
        width: { base: 216, sm: navbarCollapsed ? 64 : 216 },
        breakpoint: 'sm',
        collapsed: { mobile: !mobileOpened },
      }}
      padding={{ base: 'sm', sm: 'md' }}
    >
      <AppShell.Header hiddenFrom="sm">
        <Group h="100%" px="md">
          <Burger
            aria-label="Navigation umschalten"
            opened={mobileOpened}
            size="sm"
            onClick={() => setMobileOpened((opened) => !opened)}
          />
          <Text fw={700}>Misa</Text>
        </Group>
      </AppShell.Header>

      <AppShell.Navbar p={navbarCollapsed ? 'xs' : 'md'}>
        <AppShell.Section>
          <Title order={3} ta={navbarCollapsed ? 'center' : 'left'}>
            {navbarCollapsed ? (
              <>
                <Text component="span" hiddenFrom="sm" inherit>
                  Misa
                </Text>
                <Text component="span" visibleFrom="sm" inherit>
                  M
                </Text>
              </>
            ) : (
              'Misa'
            )}
          </Title>
        </AppShell.Section>

        <AppShell.Section grow mt="xl">
          <Stack gap="xs">
            <NavLink
              active={pathname.startsWith('/tasks')}
              aria-label="Aufgaben"
              component={Link}
              label={tasksLabel}
              to="/tasks"
              onClick={() => setMobileOpened(false)}
            />
            <NavLink
              active={pathname.startsWith('/continuum')}
              aria-label="Continuum"
              component={Link}
              label={continuumLabel}
              to="/continuum"
              onClick={() => setMobileOpened(false)}
            />
          </Stack>
        </AppShell.Section>

        <AppShell.Section visibleFrom="sm">
          <ActionIcon
            aria-label={
              navbarCollapsed
                ? 'Navigation ausklappen'
                : 'Navigation einklappen'
            }
            display="block"
            mx={navbarCollapsed ? 'auto' : 0}
            variant="subtle"
            onClick={() => setNavbarCollapsed((collapsed) => !collapsed)}
          >
            {navbarCollapsed ? '›' : '‹'}
          </ActionIcon>
        </AppShell.Section>
      </AppShell.Navbar>

      <AppShell.Main
        style={{ height: '100dvh', minWidth: 0, overflow: 'auto' }}
      >
        <Outlet />
      </AppShell.Main>
    </AppShell>
  )
}
