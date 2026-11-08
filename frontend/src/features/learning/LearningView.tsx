import { useEffect, useRef, useState } from 'react'
import {
  Alert,
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
  return `${hours} Std.`
}

export function LearningView() {
  const flashcards = useDueFlashcards()
  const refetchDueFlashcards = flashcards.refetch
  const reviewFlashcard = useReviewFlashcard()
  const [answeredCount, setAnsweredCount] = useState(0)
  const [answerVisible, setAnswerVisible] = useState(false)
  const [hintVisible, setHintVisible] = useState(false)
  const [sessionEnded, setSessionEnded] = useState(false)
  const [pendingReviews, setPendingReviews] = useState(
    () => new Map<number, number>(),
  )
  const [now, setNow] = useState(0)
  const startedAtRef = useRef(0)
  const currentFlashcard = sessionEnded ? undefined : flashcards.data?.[0]
  const preview = useFlashcardPreview(
    currentFlashcard?.id ?? 0,
    answerVisible && currentFlashcard !== undefined,
  )
  const nextDueAt = pendingReviews.size === 0
    ? undefined
    : Math.min(...pendingReviews.values())
  const dueFlashcardIds = new Set(
    flashcards.data?.map((flashcard) => flashcard.id),
  )
  const remainingCount = new Set([
    ...dueFlashcardIds,
    ...pendingReviews.keys(),
  ]).size

  useEffect(() => {
    startedAtRef.current = performance.now()
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
      sessionEnded ||
      currentFlashcard !== undefined ||
      nextDueAt === undefined ||
      now < nextDueAt
    ) return

    void refetchDueFlashcards()
  }, [currentFlashcard, nextDueAt, now, refetchDueFlashcards, sessionEnded])

  const rate = (rating: number, ratedAt: number) => {
    if (currentFlashcard === undefined || preview.data === undefined) return

    const intervals = [
      preview.data.again,
      preview.data.hard,
      preview.data.good,
      preview.data.easy,
    ]
    const isLearning = currentFlashcard.state !== 2
    const remainsInLearning =
      rating === 1 ||
      (isLearning && (
        rating === 2 ||
        (rating === 3 && currentFlashcard.learning_step === 0)
      ))
    const nextDueAt = performance.timeOrigin
      + ratedAt
      + intervals[rating - 1] * 24 * 60 * 60 * 1_000

    reviewFlashcard.mutate(
      {
        duration_ms: Math.max(0, Math.round(ratedAt - startedAtRef.current)),
        id: currentFlashcard.id,
        rating,
      },
      {
        onSuccess: () => {
          setPendingReviews((currentReviews) => {
            const nextReviews = new Map(currentReviews)
            if (remainsInLearning) {
              nextReviews.set(currentFlashcard.id, nextDueAt)
            } else {
              nextReviews.delete(currentFlashcard.id)
            }
            return nextReviews
          })
          setAnsweredCount((count) => count + 1)
          setAnswerVisible(false)
          setHintVisible(false)
          void refetchDueFlashcards()
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
        Die fälligen Lernkarten konnten nicht geladen werden.
      </Alert>
    )
  }

  if (sessionEnded) {
    return (
      <Center mih="70vh">
        <Stack align="center" gap="xs">
          <IconCards size={40} stroke={1.5} />
          <Title order={2}>Lernsession beendet</Title>
          <Text c="dimmed">
            Du hast in dieser Sitzung {answeredCount} Lernkarten beantwortet.
          </Text>
          <Text c="dimmed" size="sm">
            {nextDueAt === undefined
              ? 'Keine kurzfristige Wiederholung ausstehend.'
              : now === 0
                ? 'Wartezeit wird berechnet …'
                : now >= nextDueAt
                ? 'Nächste Wiederholung: jetzt fällig'
                : `Nächste Wiederholung in ${waitTimeLabel(nextDueAt - now)}`}
          </Text>
        </Stack>
      </Center>
    )
  }

  if (currentFlashcard === undefined && nextDueAt !== undefined) {
    return (
      <Center mih="70vh">
        <Stack align="center" gap="xs">
          <IconClock size={40} stroke={1.5} />
          <Title order={2}>Kurze Pause</Title>
          <Text c="dimmed">
            {now === 0
              ? 'Wartezeit wird berechnet …'
              : now >= nextDueAt
              ? 'Die nächste Lernkarte wird geladen …'
              : `Nächste Lernkarte in ${waitTimeLabel(nextDueAt - now)}`}
          </Text>
          {flashcards.isFetching && <Loader mt="xs" size="sm" />}
          <Button mt="md" variant="default" onClick={() => setSessionEnded(true)}>
            Lernsession beenden
          </Button>
        </Stack>
      </Center>
    )
  }

  if (currentFlashcard === undefined) {
    return (
      <Center mih="70vh">
        <Stack align="center" gap="xs">
          <IconCards size={40} stroke={1.5} />
          <Title order={2}>Lernen abgeschlossen</Title>
          <Text c="dimmed">
            Du hast in dieser Sitzung {answeredCount} Lernkarten beantwortet.
          </Text>
        </Stack>
      </Center>
    )
  }

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
        <Group gap="sm">
          <Stack align="flex-end" gap={0}>
            <Text c="dimmed" size="sm">
              {remainingCount} verbleibend
            </Text>
            {nextDueAt !== undefined && (
              <Text c="dimmed" size="xs">
                {now === 0
                  ? 'Wartezeit wird berechnet …'
                  : now >= nextDueAt
                  ? 'Nächste Wiederholung: jetzt fällig'
                  : `Nächste Wiederholung in ${waitTimeLabel(nextDueAt - now)}`}
              </Text>
            )}
          </Stack>
          <Button
            size="compact-sm"
            variant="default"
            onClick={() => setSessionEnded(true)}
          >
            Beenden
          </Button>
        </Group>
      </Group>

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
    </Box>
  )
}
