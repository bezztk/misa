<script lang="ts">
    import {createTask, getTaskMetadata} from "../../../lib/api/tasks";
    import {getDevelopmentAreas} from "../../../lib/api/developmentAreas";
    import type {DevelopmentArea} from "../../../lib/models/developmentArea";
    import type {TaskMetadata} from "../../../lib/models/taskMetadata";
    import {onMount} from "svelte";

    let {initialGroupId, development, onCancel, onCreated}: {
        initialGroupId: number | null;
        development: boolean;
        onCancel: () => void;
        onCreated: () => void;
    } = $props();

    let name = $state("");
    let description = $state("");
    let error = $state("");
    let submitting = $state(false);

    let metadata: TaskMetadata | null = $state(null);
    let areas: DevelopmentArea[] = $state([]);
    let groupId: number | null = $state(null);
    let areaId: number | null = $state(null);

    onMount(async () => {
        try {
            [metadata, areas] = await Promise.all([
                getTaskMetadata(),
                development ? getDevelopmentAreas() : Promise.resolve([]),
            ]);

            if (development && areas.length === 0) {
                throw new Error("Keine Entwicklungsbereiche verfügbar.");
            }

            if (!development && metadata.groups.length === 0) {
                throw new Error("Keine Aufgabengruppen verfügbar.");
            }

            groupId = development ? null : initialGroupId ?? metadata.groups[0].id;
            areaId = areas[0]?.id ?? null;
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
                group_id: development ? null : groupId,
                area_id: development ? areaId : null,
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

    {#if development}
        <label for="area">Entwicklungsbereich</label>
        <select id="area" bind:value={areaId}>
            {#each areas as area}
                <option value={area.id}>{area.name}</option>
            {/each}
        </select>
    {:else}
        <label for="group">Gruppe</label>
        <select id="group" bind:value={groupId}>
            {#each metadata?.groups ?? [] as group}
                <option value={group.id}>{group.name}</option>
            {/each}
        </select>
    {/if}

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
