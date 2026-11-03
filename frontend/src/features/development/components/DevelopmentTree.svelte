<script lang="ts">
    import {onMount} from "svelte";
    import {getDevelopmentCategories} from "../../../lib/api/developmentCategories";
    import {getGoals} from "../../../lib/api/goals";
    import {getItemHierarchy, updateItemParent} from "../../../lib/api/items";
    import {getSkills} from "../../../lib/api/skills";
    import {getTasks} from "../../../lib/api/tasks";

    let {areaId}: {areaId: number} = $props();

    interface TreeItem {
        id: number;
        name: string;
        type: "Category" | "Skill" | "Goal" | "Task";
        parentId: number | null;
    }

    interface TreeRow extends TreeItem {
        depth: number;
    }

    let items: TreeItem[] = $state([]);
    let rows: TreeRow[] = $state([]);
    let error = $state("");
    let formError = $state("");
    let pending = $state(false);
    let editingItem: TreeItem | null = $state(null);
    let parentId: number | null = $state(null);
    let dialog: HTMLDialogElement;

    async function loadTree() {
        try {
            error = "";

            const [categories, skills, goals, tasks, hierarchy] = await Promise.all([
                getDevelopmentCategories(),
                getSkills(),
                getGoals(),
                getTasks(),
                getItemHierarchy(),
            ]);

            const nextItems: TreeItem[] = [
                ...categories
                    .filter(category => category.area_id === areaId)
                    .map(category => ({
                        id: category.item_id,
                        name: category.name,
                        type: "Category" as const,
                        parentId: null,
                    })),
                ...skills
                    .filter(skill => skill.area_id === areaId)
                    .map(skill => ({
                        id: skill.item_id,
                        name: skill.name,
                        type: "Skill" as const,
                        parentId: null,
                    })),
                ...goals
                    .filter(goal => goal.area_id === areaId)
                    .map(goal => ({
                        id: goal.item_id,
                        name: goal.name,
                        type: "Goal" as const,
                        parentId: null,
                    })),
                ...tasks
                    .filter(task => task.area_id === areaId)
                    .map(task => ({
                        id: task.item_id,
                        name: task.name,
                        type: "Task" as const,
                        parentId: null,
                    })),
            ];

            const itemIds = new Set(nextItems.map(item => item.id));
            const parents = new Map(hierarchy.map(item => [item.child_id, item.parent_id]));

            for (const item of nextItems) {
                const parentId = parents.get(item.id);
                item.parentId = parentId !== undefined && itemIds.has(parentId) ? parentId : null;
            }

            nextItems.sort((a, b) => a.name.localeCompare(b.name));
            items = nextItems;

            const nextRows: TreeRow[] = [];

            function addChildren(parentId: number | null, depth: number) {
                for (const item of nextItems.filter(item => item.parentId === parentId)) {
                    nextRows.push({...item, depth});
                    addChildren(item.id, depth + 1);
                }
            }

            addChildren(null, 0);
            rows = nextRows;
        } catch (err) {
            error = String(err);
        }
    }

    function openParentDialog(item: TreeItem) {
        formError = "";
        editingItem = item;
        parentId = item.parentId;
        dialog.showModal();
    }

    async function saveParent() {
        if (!editingItem || pending) return;

        try {
            formError = "";
            pending = true;
            await updateItemParent(editingItem.id, parentId);
            dialog.close();
            await loadTree();
        } catch (err) {
            formError = String(err);
        } finally {
            pending = false;
        }
    }

    onMount(() => {
        void loadTree();
    });
</script>

<section>
    <h3>Hierarchie</h3>

    {#if error}
        <p>{error}</p>
    {:else if rows.length === 0}
        <p>Keine Elemente vorhanden.</p>
    {:else}
        {#each rows as row (row.id)}
            <div style={`padding-left: ${row.depth * 1.5}rem`}>
                {row.name} ({row.type})
                <button onclick={() => openParentDialog(row)}>Parent ändern</button>
            </div>
        {/each}
    {/if}
</section>

<dialog bind:this={dialog}>
    <form onsubmit={(event) => {
        event.preventDefault();
        void saveParent();
    }}>
        <h3>Parent für {editingItem?.name}</h3>

        <label for="parent">Parent</label>
        <select id="parent" bind:value={parentId}>
            <option value={null}>Kein Parent</option>
            {#each items.filter(item => item.id !== editingItem?.id) as item (item.id)}
                <option value={item.id}>{item.name} ({item.type})</option>
            {/each}
        </select>

        {#if formError}
            <p>{formError}</p>
        {/if}

        <button type="submit" disabled={pending}>
            {pending ? "Wird gespeichert..." : "Speichern"}
        </button>
        <button type="button" onclick={() => dialog.close()}>Abbrechen</button>
    </form>
</dialog>

<style>
    div {
        margin: 0.25rem 0;
    }

    div button {
        margin-left: 0.5rem;
    }

    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>
