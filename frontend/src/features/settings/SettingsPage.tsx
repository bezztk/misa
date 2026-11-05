import { useState } from 'react'
import {
  Alert,
  Box,
  Button,
  Divider,
  Group,
  Modal,
  Stack,
  Text,
  Title,
} from '@mantine/core'
import { useSeedDevelopmentData } from './api'

export function SettingsPage() {
  const [confirmationOpened, setConfirmationOpened] = useState(false)
  const seedDevelopmentData = useSeedDevelopmentData()

  const openConfirmation = () => {
    seedDevelopmentData.reset()
    setConfirmationOpened(true)
  }

  return (
    <Stack gap="md">
      <Box>
        <Title>Einstellungen</Title>
        <Divider mt="md" />
      </Box>

      <Box>
        <Text mb="sm">Testdaten für Aufgaben und Kalender erzeugen.</Text>
        <Button onClick={openConfirmation}>Testdaten generieren</Button>
      </Box>

      {seedDevelopmentData.isSuccess && (
        <Alert color="green" title="Erfolgreich">
          Die Testdaten wurden generiert.
        </Alert>
      )}
      {seedDevelopmentData.isError && (
        <Alert color="red" title="Fehler">
          Die Testdaten konnten nicht generiert werden.
        </Alert>
      )}

      <Modal
        centered
        opened={confirmationOpened}
        onClose={() => setConfirmationOpened(false)}
        title="Testdaten generieren"
      >
        <Stack>
          <Text>
            Alle bestehenden Aufgaben und Kalendereinträge werden gelöscht.
            Möchtest du fortfahren?
          </Text>
          <Group justify="flex-end">
            <Button
              disabled={seedDevelopmentData.isPending}
              variant="default"
              onClick={() => setConfirmationOpened(false)}
            >
              Abbrechen
            </Button>
            <Button
              color="red"
              loading={seedDevelopmentData.isPending}
              onClick={() =>
                seedDevelopmentData.mutate(undefined, {
                  onSettled: () => setConfirmationOpened(false),
                })
              }
            >
              Daten löschen und generieren
            </Button>
          </Group>
        </Stack>
      </Modal>
    </Stack>
  )
}
