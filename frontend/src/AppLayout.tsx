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
  Tooltip,
} from '@mantine/core'
import { useMediaQuery } from '@mantine/hooks'
import {
  IconLayoutSidebarLeftCollapse,
  IconLayoutSidebarLeftExpand,
  IconListCheck,
} from '@tabler/icons-react'
import { Link, Outlet, useRouterState } from '@tanstack/react-router'

export function AppLayout() {
  const [mobileOpened, setMobileOpened] = useState(false)
  const [navbarCollapsed, setNavbarCollapsed] = useState(false)
  const isDesktop = useMediaQuery('(min-width: 48em)')
  const compact = isDesktop && navbarCollapsed
  const pathname = useRouterState({
    select: (state) => state.location.pathname,
  })

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

      <AppShell.Navbar p={compact ? 'xs' : 'md'}>
        <AppShell.Section>
          <Group gap={compact ? 2 : 'xs'} justify="space-between" wrap="nowrap">
            <Title order={3}>{compact ? 'M' : 'Misa'}</Title>
            <ActionIcon
              aria-label={
                navbarCollapsed
                  ? 'Navigation ausklappen'
                  : 'Navigation einklappen'
              }
              size="sm"
              variant="subtle"
              visibleFrom="sm"
              onClick={() => setNavbarCollapsed((collapsed) => !collapsed)}
            >
              {navbarCollapsed ? (
                <IconLayoutSidebarLeftExpand size={18} />
              ) : (
                <IconLayoutSidebarLeftCollapse size={18} />
              )}
            </ActionIcon>
          </Group>
        </AppShell.Section>

        <AppShell.Section grow mt="lg">
          <Stack gap={2}>
            <Tooltip disabled={!compact} label="Aufgaben" position="right">
              <NavLink
                active={pathname.startsWith('/tasks')}
                aria-label="Aufgaben"
                color="gray"
                component={Link}
                label={compact ? undefined : 'Aufgaben'}
                leftSection={<IconListCheck size={20} stroke={1.5} />}
                styles={
                  compact
                    ? {
                        body: { display: 'none' },
                        root: { paddingInline: 0 },
                        section: { marginInline: 'auto' },
                      }
                    : undefined
                }
                to="/tasks"
                variant="light"
                w="100%"
                onClick={() => setMobileOpened(false)}
              />
            </Tooltip>
          </Stack>
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
