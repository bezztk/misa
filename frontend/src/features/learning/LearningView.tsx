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
import { IconBulb, IconCards } from '@tabler/icons-react'
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
  return days === 1 ? '1 Tag' : `${days} Tage`
}

export function LearningView() {
  const flashcards = useDueFlashcards()
  const reviewFlashcard = useReviewFlashcard()
  const [answeredCount, setAnsweredCount] = useState(0)
  const [answerVisible, setAnswerVisible] = useState(false)
  const [hintVisible, setHintVisible] = useState(false)
  const startedAtRef = useRef(0)
  const currentFlashcard = flashcards.data?.[0]
  const preview = useFlashcardPreview(
    currentFlashcard?.id ?? 0,
    answerVisible && currentFlashcard !== undefined,
  )

  useEffect(() => {
    startedAtRef.current = performance.now()
  }, [currentFlashcard?.id])

  const rate = (rating: number, ratedAt: number) => {
    if (currentFlashcard === undefined) return

    reviewFlashcard.mutate(
      {
        duration_ms: Math.max(0, Math.round(ratedAt - startedAtRef.current)),
        id: currentFlashcard.id,
        rating,
      },
      {
        onSuccess: () => {
          setAnsweredCount((count) => count + 1)
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
        Die fälligen Lernkarten konnten nicht geladen werden.
      </Alert>
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
        <Text c="dimmed" size="sm">
          {flashcards.data.length} verbleibend
        </Text>
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
