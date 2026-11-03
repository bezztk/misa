<script lang="ts">
    import {onMount} from "svelte";
    import {getDevelopmentCategories} from "../../../lib/api/developmentCategories";
    import {updateItemParent} from "../../../lib/api/items";
    import {getTaskMetadata, updateTask} from "../../../lib/api/tasks";
    import type {DevelopmentCategory} from "../../../lib/models/developmentCategory";
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
    let categories: DevelopmentCategory[] = $state([]);
    let categoryId: number | null = $state(null);
    let categoryChanged = $state(false);

    onMount(async () => {
        name = task.name;
        description = task.description ?? "";
        groupId = task.group_id;
        stateId = task.state_id;
        const [nextMetadata, nextCategories] = await Promise.all([
            getTaskMetadata(),
            getDevelopmentCategories(),
        ]);
        metadata = nextMetadata;
        categories = nextCategories.filter(category => category.area_id === task.area_id);
        categoryId = categories.some(category => category.item_id === task.parent_id)
            ? task.parent_id
            : null;
    });

    async function submit() {
        await updateTask(task.item_id, {
            name,
            description,
            group_id: task.area_id === null ? groupId : undefined,
            state_id: stateId,
        });

        if (task.area_id !== null && categoryChanged) {
            await updateItemParent(task.item_id, categoryId);
        }

        await onUpdated();
    }
</script>

<form onsubmit={(event) => {
    event.preventDefault();
    void submit();
}}>
    <label for="edit-name">Name</label>
    <input id="edit-name" bind:value={name} required />

    {#if task.area_id === null}
        <label for="edit-group">Gruppe</label>
        <select id="edit-group" bind:value={groupId}>
            {#each metadata?.groups ?? [] as group}
                <option value={group.id}>{group.name}</option>
            {/each}
        </select>
    {:else}
        <label for="edit-category">Kategorie</label>
        <select
            id="edit-category"
            bind:value={categoryId}
            onchange={() => categoryChanged = true}
        >
            <option value={null}>Keine Kategorie</option>
            {#each categories as category}
                <option value={category.item_id}>{category.name}</option>
            {/each}
        </select>
    {/if}

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
