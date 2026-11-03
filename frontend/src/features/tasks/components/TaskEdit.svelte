<script lang="ts">
    import {onMount} from "svelte";
    import {getTaskMetadata, updateTask} from "../../../lib/api/tasks";
    import type {Task} from "../../../lib/models/task";
    import type {TaskMetadata} from "../../../lib/models/taskMetadata";

    let {task, onCancel, onUpdated}: {
        task: Task;
        onCancel: () => void;
        onUpdated: () => Promise<void>;
    } = $props();

    let name = $state("");
    let description = $state("");
    let groupId: number | null = $state(null);
    let stateId: number | null = $state(null);
    let metadata: TaskMetadata | null = $state(null);

    onMount(async () => {
        name = task.name;
        description = task.description ?? "";
        groupId = task.group_id;
        stateId = task.state_id;
        metadata = await getTaskMetadata();
    });

    async function submit() {
        await updateTask(task.item_id, {
            name,
            description,
            group_id: groupId,
            state_id: stateId,
        });

        await onUpdated();
    }
</script>

<form onsubmit={(event) => {
    event.preventDefault();
    void submit();
}}>
    <label for="edit-name">Name</label>
    <input id="edit-name" bind:value={name} required />

    <label for="edit-group">Gruppe</label>
    <select id="edit-group" bind:value={groupId}>
        {#each metadata?.groups ?? [] as group}
            <option value={group.id}>{group.name}</option>
        {/each}
    </select>

    <label for="edit-state">Status</label>
    <select id="edit-state" bind:value={stateId}>
        {#each metadata?.states ?? [] as state}
            <option value={state.id}>{state.name}</option>
        {/each}
    </select>

    <label for="edit-description">Beschreibung</label>
    <textarea id="edit-description" bind:value={description}></textarea>

    <button type="submit">Speichern</button>
    <button type="button" onclick={onCancel}>Abbrechen</button>
</form>

<style>
    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>
