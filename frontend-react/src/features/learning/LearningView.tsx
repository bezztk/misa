import { useEffect, useRef, useState } from 'react'
import {
  Alert,
  Badge,
  Box,
  Button,
  Center,
  Group,
  Loader,
  Paper,
  SimpleGrid,
  Stack,
  Text,
  Title,
} from '@mantine/core'
import { Crepe } from '@milkdown/crepe'
import '@milkdown/crepe/theme/common/style.css'
import '@milkdown/crepe/theme/frame.css'
import { IconBulb, IconCards, IconClock } from '@tabler/icons-react'
import type { Flashcard } from '../realization/types'
import { useDueFlashcards, useFlashcardPreview, useReviewFlashcard } from './api'
import './LearningView.css'

function MarkdownContent({ markdown }: { markdown: string }) {
  const rootRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    if (rootRef.current === null) return

    const crepe = new Crepe({
      defaultValue: markdown,
      features: {
        [Crepe.Feature.BlockEdit]: false,
        [Crepe.Feature.Cursor]: false,
        [Crepe.Feature.LinkTooltip]: false,
        [Crepe.Feature.Placeholder]: false,
        [Crepe.Feature.Toolbar]: false,
      },
      root: rootRef.current,
    }).setReadonly(true)
    const createPromise = crepe.create()

    return () => {
      void createPromise.then(() => crepe.destroy())
    }
  }, [markdown])

  return <div ref={rootRef} className="learning-markdown" />
}

function intervalLabel(days: number) {
  const minutes = days * 24 * 60
  if (minutes < 60) {
    return `${Math.max(1, Math.round(minutes))} Min.`
  }

  const hours = minutes / 60
  if (hours < 24) {
    const roundedHours = Math.round(hours * 10) / 10
    return `${roundedHours.toLocaleString('de-DE')} Std.`
  }

  const roundedDays = Math.round(days * 10) / 10
  return roundedDays === 1
    ? '1 Tag'
    : `${roundedDays.toLocaleString('de-DE')} Tage`
}

function waitTimeLabel(milliseconds: number) {
  const seconds = Math.max(0, Math.ceil(milliseconds / 1_000))
  if (seconds < 60) return `${seconds} Sek.`

  const minutes = Math.ceil(seconds / 60)
  if (minutes < 60) return `${minutes} Min.`

  const hours = Math.ceil(minutes / 60)
  if (hours < 24) return `${hours} Std.`

  const days = Math.ceil(hours / 24)
  return days === 1 ? '1 Tag' : `${days} Tage`
}

function stateDetails(state: number) {
  switch (state) {
    case 1:
      return { color: 'blue', label: 'Lernen' }
    case 2:
      return { color: 'grape', label: 'Wiederholung' }
    case 3:
      return { color: 'orange', label: 'Wiederlernen' }
    default:
      return { color: 'gray', label: 'Neu' }
  }
}

function QueueCard({
  active = false,
  card,
  dueLabel,
}: {
  active?: boolean
  card: Flashcard
  dueLabel?: string
}) {
  const state = stateDetails(card.state)

  return (
    <Group
      className="learning-queue-card"
      data-active={active || undefined}
      gap="sm"
      justify="space-between"
      wrap="nowrap"
    >
      <Text className="learning-question-preview" fw={active ? 600 : 400} size="sm">
        {card.front}
      </Text>
      <Group gap="xs" wrap="nowrap">
        {dueLabel !== undefined && (
          <Text c="dimmed" size="xs" style={{ whiteSpace: 'nowrap' }}>
            {dueLabel}
          </Text>
        )}
        <Badge color={state.color} size="sm" variant="light">
          {state.label}
        </Badge>
      </Group>
    </Group>
  )
}

export function LearningView() {
  const flashcards = useDueFlashcards()
  const reviewFlashcard = useReviewFlashcard()
  const [answerVisible, setAnswerVisible] = useState(false)
  const [hintVisible, setHintVisible] = useState(false)
  const [now, setNow] = useState(0)
  const startedAtRef = useRef(0)
  const isFetching = flashcards.isFetching
  const refetchFlashcards = flashcards.refetch
  const currentFlashcard = flashcards.data?.due[0]
  const nextUpcoming = flashcards.data?.upcoming[0]
  const nextDueAt = nextUpcoming?.due_at === null || nextUpcoming === undefined
    ? undefined
    : new Date(nextUpcoming.due_at).getTime()
  const preview = useFlashcardPreview(
    currentFlashcard?.id ?? 0,
    answerVisible && currentFlashcard !== undefined,
  )

  useEffect(() => {
    const timer = setTimeout(() => {
      startedAtRef.current = performance.now()
      setAnswerVisible(false)
      setHintVisible(false)
    }, 0)
    return () => clearTimeout(timer)
  }, [currentFlashcard?.id])

  useEffect(() => {
    if (nextDueAt === undefined) return

    const initialTimer = setTimeout(() => setNow(Date.now()), 0)
    const timer = setInterval(() => setNow(Date.now()), 1_000)
    return () => {
      clearTimeout(initialTimer)
      clearInterval(timer)
    }
  }, [nextDueAt])

  useEffect(() => {
    if (
      nextDueAt === undefined ||
      now < nextDueAt ||
      isFetching
    ) return

    void refetchFlashcards()
  }, [isFetching, nextDueAt, now, refetchFlashcards])

  const rate = (rating: number, ratedAt: number) => {
    if (currentFlashcard === undefined || preview.data === undefined) return

    reviewFlashcard.mutate(
      {
        duration_ms: Math.max(0, Math.round(ratedAt - startedAtRef.current)),
        id: currentFlashcard.id,
        rating,
      },
      {
        onSuccess: () => {
          setAnswerVisible(false)
          setHintVisible(false)
        },
      },
    )
  }

  if (flashcards.isPending) {
    return <Center h="100%"><Loader /></Center>
  }

  if (flashcards.isError) {
    return (
      <Alert color="red" title="Fehler">
        Die Lernkarten-Warteschlange konnte nicht geladen werden.
      </Alert>
    )
  }

  const due = flashcards.data.due
  const upcoming = flashcards.data.upcoming
  const ratings = preview.data === undefined ? [] : [
    { color: 'red', interval: preview.data.again, label: 'Nicht gewusst', rating: 1 },
    { color: 'orange', interval: preview.data.hard, label: 'Schwer', rating: 2 },
    { color: 'blue', interval: preview.data.good, label: 'Gut', rating: 3 },
    { color: 'green', interval: preview.data.easy, label: 'Einfach', rating: 4 },
  ]

  return (
    <Box maw={840} mx="auto">
      <Group justify="space-between" mb="md">
        <Title order={2}>Lernen</Title>
        <Group gap="xs">
          <Badge color="blue" variant="light">{due.length} fällig</Badge>
          <Badge color="gray" variant="light">{upcoming.length} demnächst</Badge>
        </Group>
      </Group>

      {currentFlashcard === undefined ? (
        <Paper p="xl" withBorder>
          <Center>
            <Stack align="center" gap="xs">
              {nextDueAt === undefined ? (
                <IconCards size={40} stroke={1.5} />
              ) : (
                <IconClock size={40} stroke={1.5} />
              )}
              <Title order={3}>
                {nextDueAt === undefined ? 'Alles erledigt' : 'Aktuell nichts fällig'}
              </Title>
              <Text c="dimmed">
                {nextDueAt === undefined
                  ? 'Es stehen keine weiteren Lernkarten an.'
                  : now === 0
                    ? 'Wartezeit wird berechnet …'
                    : now >= nextDueAt
                    ? 'Die nächste Lernkarte wird geladen …'
                    : `Nächste Wiederholung in ${waitTimeLabel(nextDueAt - now)}`}
              </Text>
              {flashcards.isFetching && <Loader mt="xs" size="sm" />}
            </Stack>
          </Center>
        </Paper>
      ) : (
        <Paper p={{ base: 'md', sm: 'xl' }} withBorder>
          <Stack gap="lg">
            <Box>
              <Text c="dimmed" mb="xs" size="xs" tt="uppercase">
                Vorderseite
              </Text>
              <MarkdownContent markdown={currentFlashcard.front} />
            </Box>

            {currentFlashcard.hint !== null && !hintVisible && !answerVisible && (
              <Button
                leftSection={<IconBulb size={16} />}
                size="compact-sm"
                variant="subtle"
                w="fit-content"
                onClick={() => setHintVisible(true)}
              >
                Hinweis anzeigen
              </Button>
            )}
            {hintVisible && currentFlashcard.hint !== null && !answerVisible && (
              <Paper bg="var(--mantine-color-gray-0)" p="sm" withBorder>
                <Text c="dimmed" mb={4} size="xs">Hinweis</Text>
                <MarkdownContent markdown={currentFlashcard.hint} />
              </Paper>
            )}

            {!answerVisible ? (
              <Button onClick={() => setAnswerVisible(true)}>
                Antwort anzeigen
              </Button>
            ) : (
              <Stack gap="lg">
                <Box>
                  <Text c="dimmed" mb="xs" size="xs" tt="uppercase">
                    Rückseite
                  </Text>
                  <MarkdownContent markdown={currentFlashcard.back} />
                </Box>

                {currentFlashcard.explanation !== null && (
                  <Paper bg="var(--mantine-color-gray-0)" p="sm" withBorder>
                    <Text c="dimmed" mb={4} size="xs">Erklärung</Text>
                    <MarkdownContent markdown={currentFlashcard.explanation} />
                  </Paper>
                )}

                {preview.isPending ? (
                  <Center py="sm"><Loader size="sm" /></Center>
                ) : preview.isError ? (
                  <Alert color="red" title="Fehler">
                    Die Wiederholungsintervalle konnten nicht geladen werden.
                  </Alert>
                ) : (
                  <SimpleGrid cols={{ base: 2, sm: 4 }} spacing="xs">
                    {ratings.map((rating) => (
                      <Button
                        key={rating.rating}
                        color={rating.color}
                        disabled={reviewFlashcard.isPending}
                        h="auto"
                        py="xs"
                        variant="light"
                        onClick={(event) => rate(rating.rating, event.timeStamp)}
                      >
                        <Stack gap={0} align="center">
                          <Text fw={600} size="sm">{rating.label}</Text>
                          <Text size="xs">{intervalLabel(rating.interval)}</Text>
                        </Stack>
                      </Button>
                    ))}
                  </SimpleGrid>
                )}

                {reviewFlashcard.isError && (
                  <Alert color="red" title="Fehler">
                    Die Bewertung konnte nicht gespeichert werden.
                  </Alert>
                )}
              </Stack>
            )}
          </Stack>
        </Paper>
      )}

      <Stack gap="xs" mt="xl">
        <Group justify="space-between">
          <Title order={4}>Warteschlange</Title>
          <Text c="dimmed" size="sm">{due.length} Karten</Text>
        </Group>
        <Paper className="learning-queue" withBorder>
          {due.length === 0 ? (
            <Text c="dimmed" p="md" size="sm">Aktuell ist keine Karte fällig.</Text>
          ) : due.map((card, index) => (
            <QueueCard key={card.id} active={index === 0} card={card} />
          ))}
        </Paper>
      </Stack>

      <Stack gap="xs" mt="xl" mb="xl">
        <Group justify="space-between">
          <Title order={4}>Demnächst</Title>
          <Text c="dimmed" size="sm">Nächste {upcoming.length} Karten</Text>
        </Group>
        <Paper className="learning-queue" withBorder>
          {upcoming.length === 0 ? (
            <Text c="dimmed" p="md" size="sm">Keine weiteren Wiederholungen geplant.</Text>
          ) : upcoming.map((card) => {
            const dueAt = card.due_at === null ? now : new Date(card.due_at).getTime()
            return (
              <QueueCard
                key={card.id}
                card={card}
                dueLabel={now === 0
                  ? 'wird berechnet …'
                  : dueAt <= now
                    ? 'jetzt'
                    : `in ${waitTimeLabel(dueAt - now)}`}
              />
            )
          })}
        </Paper>
      </Stack>
    </Box>
  )
}
