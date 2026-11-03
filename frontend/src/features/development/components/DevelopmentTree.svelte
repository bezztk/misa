<script lang="ts">
    import {onMount} from "svelte";
    import {
        createDevelopmentCategory,
        deleteDevelopmentCategory,
        getDevelopmentCategories,
        updateDevelopmentCategory,
    } from "../../../lib/api/developmentCategories";
    import {createGoal, deleteGoal, getGoals, updateGoal} from "../../../lib/api/goals";
    import {getItemHierarchy, updateItemParent} from "../../../lib/api/items";
    import {
        createSkill,
        deleteSkill,
        getSkillMetadata,
        getSkills,
        updateSkill,
    } from "../../../lib/api/skills";
    import {getTasks} from "../../../lib/api/tasks";
    import type {SkillMetadata} from "../../../lib/models/skill";

    let {areaId}: {areaId: number} = $props();

    type ItemType = "Category" | "Skill" | "Goal" | "Task";
    type EditableType = "Category" | "Skill" | "Goal";

    interface TreeItem {
        id: number;
        name: string;
        type: ItemType;
        parentId: number | null;
        description: string | null;
        stateId: number | null;
        isCompleted: boolean;
        dueAt: string | null;
    }

    interface TreeRow extends TreeItem {
        depth: number;
    }

    let items: TreeItem[] = $state([]);
    let rows: TreeRow[] = $state([]);
    let skillMetadata: SkillMetadata | null = $state(null);
    let error = $state("");
    let pending = $state(false);

    let itemDialog: HTMLDialogElement;
    let editingItem: TreeItem | null = $state(null);
    let formType: EditableType = $state("Category");
    let name = $state("");
    let description = $state("");
    let stateId: number | null = $state(null);
    let isCompleted = $state(false);
    let dueDate = $state("");
    let itemFormError = $state("");

    let parentDialog: HTMLDialogElement;
    let parentItem: TreeItem | null = $state(null);
    let parentId: number | null = $state(null);
    let parentFormError = $state("");

    async function loadTree() {
        try {
            error = "";

            const [categories, skills, goals, tasks, hierarchy, metadata] = await Promise.all([
                getDevelopmentCategories(),
                getSkills(),
                getGoals(),
                getTasks(),
                getItemHierarchy(),
                getSkillMetadata(),
            ]);

            skillMetadata = metadata;

            const nextItems: TreeItem[] = [
                ...categories
                    .filter(category => category.area_id === areaId)
                    .map(category => ({
                        id: category.item_id,
                        name: category.name,
                        type: "Category" as const,
                        parentId: null,
                        description: null,
                        stateId: null,
                        isCompleted: false,
                        dueAt: null,
                    })),
                ...skills
                    .filter(skill => skill.area_id === areaId)
                    .map(skill => ({
                        id: skill.item_id,
                        name: skill.name,
                        type: "Skill" as const,
                        parentId: null,
                        description: skill.description,
                        stateId: skill.state_id,
                        isCompleted: false,
                        dueAt: null,
                    })),
                ...goals
                    .filter(goal => goal.area_id === areaId)
                    .map(goal => ({
                        id: goal.item_id,
                        name: goal.name,
                        type: "Goal" as const,
                        parentId: null,
                        description: goal.description,
                        stateId: null,
                        isCompleted: goal.is_completed,
                        dueAt: goal.due_at,
                    })),
                ...tasks
                    .filter(task => task.area_id === areaId)
                    .map(task => ({
                        id: task.item_id,
                        name: task.name,
                        type: "Task" as const,
                        parentId: null,
                        description: task.description,
                        stateId: task.state_id,
                        isCompleted: false,
                        dueAt: null,
                    })),
            ];

            const itemIds = new Set(nextItems.map(item => item.id));
            const parents = new Map(hierarchy.map(item => [item.child_id, item.parent_id]));

            for (const item of nextItems) {
                const nextParentId = parents.get(item.id);
                item.parentId = nextParentId !== undefined && itemIds.has(nextParentId)
                    ? nextParentId
                    : null;
            }

            nextItems.sort((a, b) => a.name.localeCompare(b.name));
            items = nextItems;

            const nextRows: TreeRow[] = [];

            function addChildren(nextParentId: number | null, depth: number) {
                for (const item of nextItems.filter(item => item.parentId === nextParentId)) {
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

    function openCreate(type: EditableType) {
        itemFormError = "";
        editingItem = null;
        formType = type;
        name = "";
        description = "";
        stateId = skillMetadata?.states[0]?.id ?? null;
        isCompleted = false;
        dueDate = "";
        itemDialog.showModal();
    }

    function openEdit(item: TreeItem) {
        if (item.type === "Task") return;

        itemFormError = "";
        editingItem = item;
        formType = item.type;
        name = item.name;
        description = item.description ?? "";
        stateId = item.stateId;
        isCompleted = item.isCompleted;
        dueDate = item.dueAt?.slice(0, 10) ?? "";
        itemDialog.showModal();
    }

    async function saveItem() {
        if (pending) return;

        try {
            itemFormError = "";
            pending = true;

            if (editingItem) {
                if (formType === "Category") {
                    await updateDevelopmentCategory(editingItem.id, {name});
                } else if (formType === "Skill") {
                    await updateSkill(editingItem.id, {name, description, state_id: stateId});
                } else {
                    const dueAt = dueDate
                        ? new Date(`${dueDate}T00:00:00.000Z`).toISOString()
                        : null;
                    await updateGoal(editingItem.id, {
                        name,
                        description,
                        is_completed: isCompleted,
                        due_at: dueAt ?? undefined,
                        clear_due_at: dueAt === null,
                    });
                }
            } else if (formType === "Category") {
                await createDevelopmentCategory({area_id: areaId, name});
            } else if (formType === "Skill") {
                await createSkill({area_id: areaId, name, description, state_id: stateId});
            } else {
                const dueAt = dueDate
                    ? new Date(`${dueDate}T00:00:00.000Z`).toISOString()
                    : null;
                await createGoal({area_id: areaId, name, description, due_at: dueAt});
            }

            itemDialog.close();
            await loadTree();
        } catch (err) {
            itemFormError = String(err);
        } finally {
            pending = false;
        }
    }

    async function removeItem(item: TreeItem) {
        if (item.type === "Task" || pending || !confirm(`${item.type} "${item.name}" löschen?`)) {
            return;
        }

        try {
            error = "";
            pending = true;

            if (item.type === "Category") {
                await deleteDevelopmentCategory(item.id);
            } else if (item.type === "Skill") {
                await deleteSkill(item.id);
            } else {
                await deleteGoal(item.id);
            }

            await loadTree();
        } catch (err) {
            error = String(err);
        } finally {
            pending = false;
        }
    }

    function openParentDialog(item: TreeItem) {
        parentFormError = "";
        parentItem = item;
        parentId = item.parentId;
        parentDialog.showModal();
    }

    async function saveParent() {
        if (!parentItem || pending) return;

        try {
            parentFormError = "";
            pending = true;
            await updateItemParent(parentItem.id, parentId);
            parentDialog.close();
            await loadTree();
        } catch (err) {
            parentFormError = String(err);
        } finally {
            pending = false;
        }
    }

    onMount(() => {
        void loadTree();
    });
</script>

<section>
    <div class="actions">
        <button onclick={() => openCreate("Category")} disabled={pending}>Neue Kategorie</button>
        <button onclick={() => openCreate("Skill")} disabled={pending}>Neuer Skill</button>
        <button onclick={() => openCreate("Goal")} disabled={pending}>Neues Goal</button>
    </div>

    {#if error}
        <p>{error}</p>
    {:else if rows.length === 0}
        <p>Keine Elemente vorhanden.</p>
    {:else}
        {#each rows as row (row.id)}
            <div class="tree-row" style={`padding-left: ${row.depth * 1.5}rem`}>
                {row.name} ({row.type})
                <button onclick={() => openParentDialog(row)} disabled={pending}>Parent</button>
                {#if row.type !== "Task"}
                    <button onclick={() => openEdit(row)} disabled={pending}>Bearbeiten</button>
                    <button onclick={() => removeItem(row)} disabled={pending}>Löschen</button>
                {/if}
            </div>
        {/each}
    {/if}
</section>

<dialog bind:this={itemDialog}>
    <form onsubmit={(event) => {
        event.preventDefault();
        void saveItem();
    }}>
        <h3>{editingItem ? `${formType} bearbeiten` : `${formType} erstellen`}</h3>

        <label for="item-name">Name</label>
        <input id="item-name" bind:value={name} required />

        {#if formType === "Skill"}
            <label for="skill-state">Status</label>
            <select id="skill-state" bind:value={stateId} required>
                {#each skillMetadata?.states ?? [] as state}
                    <option value={state.id}>{state.name}</option>
                {/each}
            </select>
        {/if}

        {#if formType === "Skill" || formType === "Goal"}
            <label for="item-description">Beschreibung</label>
            <textarea id="item-description" bind:value={description}></textarea>
        {/if}

        {#if formType === "Goal"}
            <label for="goal-due-date">Fälligkeitsdatum</label>
            <input id="goal-due-date" type="date" bind:value={dueDate} />

            {#if editingItem}
                <label>
                    <input type="checkbox" bind:checked={isCompleted} />
                    Abgeschlossen
                </label>
            {/if}
        {/if}

        {#if itemFormError}
            <p>{itemFormError}</p>
        {/if}

        <button type="submit" disabled={pending}>
            {pending ? "Wird gespeichert..." : "Speichern"}
        </button>
        <button type="button" onclick={() => itemDialog.close()}>Abbrechen</button>
    </form>
</dialog>

<dialog bind:this={parentDialog}>
    <form onsubmit={(event) => {
        event.preventDefault();
        void saveParent();
    }}>
        <h3>Parent für {parentItem?.name}</h3>

        <label for="parent">Parent</label>
        <select id="parent" bind:value={parentId}>
            <option value={null}>Kein Parent</option>
            {#each items.filter(item => item.id !== parentItem?.id) as item (item.id)}
                <option value={item.id}>{item.name} ({item.type})</option>
            {/each}
        </select>

        {#if parentFormError}
            <p>{parentFormError}</p>
        {/if}

        <button type="submit" disabled={pending}>
            {pending ? "Wird gespeichert..." : "Speichern"}
        </button>
        <button type="button" onclick={() => parentDialog.close()}>Abbrechen</button>
    </form>
</dialog>

<style>
    .actions {
        display: flex;
        gap: 0.5rem;
        margin: 0.75rem 0;
    }

    .tree-row {
        margin: 0.25rem 0;
    }

    .tree-row button {
        margin-left: 0.5rem;
    }

    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>
