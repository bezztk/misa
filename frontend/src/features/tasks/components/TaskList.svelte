<script lang="ts">
    import {onMount} from "svelte";
    import {getDevelopmentCategories} from "../../../lib/api/developmentCategories";
    import {deleteTask, getTaskMetadata, getTasks} from "../../../lib/api/tasks";
    import type {DevelopmentCategory} from "../../../lib/models/developmentCategory";
    import type {Task} from "../../../lib/models/task";
    import type {TaskMetadata} from "../../../lib/models/taskMetadata";
    import TaskCreate from "./TaskCreate.svelte";
    import TaskEdit from "./TaskEdit.svelte";

    let tasks: Task[] = $state([]);
    let metadata: TaskMetadata | null = $state(null);
    let categories: DevelopmentCategory[] = $state([]);
    let selectedView: "all" | "development" | number = $state("all");
    let selectedTask: Task | null = $state(null);
    let editingTask: Task | null = $state(null);
    let createGroupId: number | null = $state(null);
    let creating = $state(false);
    let error = $state("");
    let createDialog: HTMLDialogElement;
    let editDialog: HTMLDialogElement;

    let visibleTasks = $derived(tasks.filter(task => {
        if (selectedView === "development") return task.area_id !== null;
        if (selectedView === "all") return task.area_id === null;
        return task.area_id === null && task.group_id === selectedView;
    }));

    export async function loadTasks() {
        try {
            error = "";
            tasks = await getTasks();
            if (selectedTask) {
                selectedTask = tasks.find(task => task.item_id === selectedTask?.item_id) ?? null;
            }
        } catch (err) {
            error = String(err);
        }
    }

    onMount(async () => {
        try {
            [metadata, categories] = await Promise.all([
                getTaskMetadata(),
                getDevelopmentCategories(),
            ]);
            await loadTasks();
        } catch (err) {
            error = String(err);
        }
    });

    function openCreate() {
        createGroupId = typeof selectedView === "number"
            ? selectedView
            : metadata?.groups[0]?.id ?? null;
        creating = true;
        createDialog.showModal();
    }

    function openEdit(task: Task) {
        editingTask = task;
        editDialog.showModal();
    }

    function selectView(view: "all" | "development" | number) {
        selectedView = view;
        selectedTask = null;
    }

    async function removeSelected() {
        if (!selectedTask) return;

        await deleteTask(selectedTask.item_id);
        selectedTask = null;
        await loadTasks();
    }
</script>

<div class="task-layout" class:with-detail={selectedTask !== null}>
    <nav class="task-navigation" aria-label="Aufgabenansichten">
        <button class:active={selectedView === "all"} onclick={() => selectView("all")}>
            Alle Aufgaben
        </button>

        <button
            class:active={selectedView === "development"}
            onclick={() => selectView("development")}
        >
            Entwicklung
        </button>

        <strong>Gruppen</strong>
        {#each metadata?.groups ?? [] as group}
            <button class:active={selectedView === group.id} onclick={() => selectView(group.id)}>
                {group.name}
            </button>
        {/each}
    </nav>

    <section>
        <div class="heading">
            <h2>Aufgaben</h2>
            {#if selectedView !== "development"}
                <button onclick={openCreate}>Neue Aufgabe</button>
            {/if}
        </div>

        {#if error}
            <p>{error}</p>
        {:else if visibleTasks.length === 0}
            <p>Keine Aufgaben vorhanden.</p>
        {:else}
            {#each visibleTasks as task (task.item_id)}
                <button
                    class="task-row"
                    class:selected={selectedTask?.item_id === task.item_id}
                    onclick={() => selectedTask = task}
                >
                    <strong>{task.name}</strong>
                    <span>{metadata?.states.find(state => state.id === task.state_id)?.name}</span>
                    {#if task.group_id !== null}
                        <span>{metadata?.groups.find(group => group.id === task.group_id)?.name}</span>
                    {/if}
                    {#if task.parent_id !== null && categories.some(category => category.item_id === task.parent_id)}
                        <span>{categories.find(category => category.item_id === task.parent_id)?.name}</span>
                    {/if}
                    {#if task.description}
                        <small>{task.description}</small>
                    {/if}
                </button>
            {/each}
        {/if}
    </section>

    {#if selectedTask}
        <aside class="task-details">
            <button class="close" aria-label="Detailbereich schließen" onclick={() => selectedTask = null}>×</button>
            <h2>{selectedTask.name}</h2>
            <p class="type">Aufgabe</p>

            {#if selectedTask.description}
                <h3>Beschreibung</h3>
                <p>{selectedTask.description}</p>
            {/if}

            <p><strong>Status:</strong> {metadata?.states.find(state => state.id === selectedTask?.state_id)?.name}</p>

            {#if selectedTask.group_id !== null}
                <p><strong>Gruppe:</strong> {metadata?.groups.find(group => group.id === selectedTask?.group_id)?.name}</p>
            {/if}

            {#if selectedTask.parent_id !== null && categories.some(category => category.item_id === selectedTask?.parent_id)}
                <p><strong>Kategorie:</strong> {categories.find(category => category.item_id === selectedTask?.parent_id)?.name}</p>
            {/if}

            {#if selectedTask.completed_at}
                <p><strong>Abgeschlossen am:</strong> {new Date(selectedTask.completed_at).toLocaleDateString("de-DE")}</p>
            {/if}

            <div class="detail-actions">
                <button onclick={() => selectedTask && openEdit(selectedTask)}>Bearbeiten</button>
                <button onclick={() => void removeSelected()}>Löschen</button>
            </div>
        </aside>
    {/if}
</div>

<dialog bind:this={createDialog} onclose={() => creating = false}>
    {#if creating}
        <TaskCreate
            initialGroupId={createGroupId}
            onCancel={() => createDialog.close()}
            onCreated={() => {
                createDialog.close();
                void loadTasks();
            }}
        />
    {/if}
</dialog>

<dialog bind:this={editDialog} onclose={() => editingTask = null}>
    {#if editingTask}
        <TaskEdit
            task={editingTask}
            onCancel={() => editDialog.close()}
            onUpdated={async () => {
                editDialog.close();
                await loadTasks();
            }}
        />
    {/if}
</dialog>

<style>
    .task-layout {
        display: grid;
        grid-template-columns: 12rem minmax(0, 1fr);
        gap: 2rem;
    }

    .task-layout.with-detail {
        grid-template-columns: 12rem minmax(18rem, 1fr) 18rem;
    }

    .task-navigation {
        display: flex;
        min-height: calc(100vh - 4rem);
        padding-right: 1rem;
        border-right: 1px solid #dddddd;
        flex-direction: column;
        gap: 0.25rem;
    }

    .task-navigation strong {
        margin: 1rem 0 0.25rem;
        padding: 0 0.5rem;
        font-size: 0.85rem;
        color: #666666;
    }

    .task-navigation button {
        padding: 0.5rem;
        border: 0;
        border-radius: 0.25rem;
        background: transparent;
        text-align: left;
        cursor: pointer;
    }

    .task-navigation button:hover,
    .task-navigation button.active {
        background: #e8e8e8;
    }

    .heading {
        display: flex;
        align-items: center;
        justify-content: space-between;
    }

    .heading h2 {
        margin-top: 0;
    }

    .task-row {
        display: grid;
        width: 100%;
        padding: 0.35rem 0.5rem;
        border: 0;
        border-bottom: 1px solid #e5e5e5;
        background: transparent;
        grid-template-columns: minmax(8rem, 1fr) auto auto auto;
        gap: 0.25rem 0.75rem;
        text-align: left;
        cursor: pointer;
    }

    .task-row:hover,
    .task-row.selected {
        background: #eeeeee;
    }

    .task-row span,
    .task-row small {
        color: #666666;
        font-size: 0.8rem;
    }

    .task-row small {
        overflow: hidden;
        grid-column: 1 / -1;
        text-overflow: ellipsis;
        white-space: nowrap;
    }

    .task-details {
        position: sticky;
        top: 2rem;
        align-self: start;
        padding-left: 1.5rem;
        border-left: 1px solid #dddddd;
    }

    .task-details h2 {
        margin: 0 2rem 0.25rem 0;
    }

    .task-details h3 {
        margin-bottom: 0.25rem;
        font-size: 1rem;
    }

    .task-details p {
        margin: 0.5rem 0;
    }

    .type {
        color: #666666;
    }

    .close {
        float: right;
        border: 0;
        background: transparent;
        font-size: 1.25rem;
        cursor: pointer;
    }

    .detail-actions {
        display: flex;
        gap: 0.5rem;
        margin-top: 1.25rem;
    }
</style>
