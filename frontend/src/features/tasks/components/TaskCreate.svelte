<script lang="ts">
    import {createTask, getTaskMetadata} from "../../../lib/api/tasks";
    import type {TaskMetadata} from "../../../lib/models/taskMetadata";
    import {onMount} from "svelte";

    let {initialGroupId, onCancel, onCreated}: {
        initialGroupId: number | null;
        onCancel: () => void;
        onCreated: () => void;
    } = $props();

    let name = $state("");
    let description = $state("");
    let error = $state("");
    let submitting = $state(false);

    let metadata: TaskMetadata | null = $state(null);
    let groupId: number | null = $state(null);

    onMount(async () => {
        try {
            metadata = await getTaskMetadata();

            if (metadata.groups.length === 0) {
                throw new Error("Keine Aufgabengruppen verfügbar.");
            }

            groupId = initialGroupId ?? metadata.groups[0].id;
        } catch (err) {
            error = String(err);
        }
    });

    async function submit() {
        try {
            error = "";
            submitting = true;

            await createTask({
                name,
                description,
                group_id: groupId
            });

            name = "";
            description = "";
            onCreated();
        } catch (err) {
            error = String(err);
        } finally {
            submitting = false;
        }
    }
</script>

<form onsubmit={(event) => {
    event.preventDefault();
    void submit();
}}>
    <input
            type="text"
            placeholder="Aufgabenname"
            bind:value={name}
            required
    />

    <label for="group">Gruppe</label>
    <select id="group" bind:value={groupId}>
        {#each metadata?.groups ?? [] as group}
            <option value={group.id}>{group.name}</option>
        {/each}
    </select>

    <textarea placeholder="Beschreibung" bind:value={description}></textarea>

    <button type="submit" disabled={submitting}>
        {submitting ? "Wird erstellt..." : "Aufgabe erstellen"}
    </button>

    <button type="button" onclick={onCancel}>Abbrechen</button>
</form>

{#if error}
    <p>{error}</p>
{/if}

<style>
    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>
