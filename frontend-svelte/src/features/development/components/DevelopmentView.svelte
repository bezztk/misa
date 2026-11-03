<script lang="ts">
    import {onMount} from "svelte";
    import {
        createDevelopmentArea,
        deleteDevelopmentArea,
        getDevelopmentAreas,
        updateDevelopmentArea,
    } from "../../../lib/api/developmentAreas";
    import type {DevelopmentArea} from "../../../lib/models/developmentArea";
    import DevelopmentTree from "./DevelopmentTree.svelte";

    type ElementType = "Development Area" | "Category" | "Skill" | "Goal" | "Task";

    interface SelectedElement {
        id: number;
        type: ElementType;
        name: string;
        description: string | null;
        stateName: string | null;
        groupName: string | null;
        isCompleted: boolean;
        completedAt: string | null;
        dueAt: string | null;
        parentName: string | null;
        edit: () => void;
        remove: () => void;
        editParent: (() => void) | null;
    }

    let areas: DevelopmentArea[] = $state([]);
    let selectedAreaId: number | null = $state(null);
    let selectedItemId: number | null = $state(null);
    let selectedElement: SelectedElement | null = $state(null);
    let editingArea: DevelopmentArea | null = $state(null);
    let name = $state("");
    let description = $state("");
    let error = $state("");
    let formError = $state("");
    let pending = $state(false);
    let dialog: HTMLDialogElement;

    async function loadAreas() {
        try {
            error = "";
            areas = await getDevelopmentAreas();

            if (selectedElement?.type === "Development Area") {
                const selectedArea = areas.find(area => area.id === selectedElement?.id);
                selectedArea ? selectArea(selectedArea) : closePanel();
            }
        } catch (err) {
            error = String(err);
        }
    }

    function closePanel() {
        selectedAreaId = null;
        selectedItemId = null;
        selectedElement = null;
    }

    function selectArea(area: DevelopmentArea) {
        selectedAreaId = area.id;
        selectedItemId = null;
        selectedElement = {
            id: area.id,
            type: "Development Area",
            name: area.name,
            description: area.description,
            stateName: null,
            groupName: null,
            isCompleted: false,
            completedAt: null,
            dueAt: null,
            parentName: null,
            edit: () => openEdit(area),
            remove: () => void remove(area),
            editParent: null,
        };
    }

    function openCreate() {
        error = "";
        formError = "";
        editingArea = null;
        name = "";
        description = "";
        dialog.showModal();
    }

    function openEdit(area: DevelopmentArea) {
        error = "";
        formError = "";
        editingArea = area;
        name = area.name;
        description = area.description ?? "";
        dialog.showModal();
    }

    async function submit() {
        if (pending) return;

        try {
            formError = "";
            pending = true;

            if (editingArea) {
                await updateDevelopmentArea(editingArea.id, {name, description});
            } else {
                await createDevelopmentArea({name, description});
            }

            dialog.close();
            await loadAreas();
        } catch (err) {
            formError = String(err);
        } finally {
            pending = false;
        }
    }

    async function remove(area: DevelopmentArea) {
        if (pending || !confirm(`Area "${area.name}" löschen?`)) return;

        try {
            error = "";
            pending = true;
            await deleteDevelopmentArea(area.id);
            if (selectedElement?.type === "Development Area" && selectedElement.id === area.id) {
                closePanel();
            }
            await loadAreas();
        } catch (err) {
            error = String(err);
        } finally {
            pending = false;
        }
    }

    onMount(() => {
        void loadAreas();
    });
</script>

<div class="development-view">
    <section class="tree-pane">
        <h2>Entwicklung</h2>
        <button onclick={openCreate} disabled={pending}>Neuer Bereich</button>

        {#if error}
            <p>{error}</p>
        {/if}

        {#each areas as area (area.id)}
            <article>
                <div class="area-heading">
                    <button
                        class="area-name"
                        class:selected={selectedAreaId === area.id}
                        onclick={() => selectArea(area)}
                    >
                        {area.name}
                    </button>
                </div>

                <DevelopmentTree
                    areaId={area.id}
                    {selectedItemId}
                    onSelect={(item, edit, remove, editParent) => {
                        selectedItemId = item.id;
                        selectedAreaId = null;
                        selectedElement = {...item, edit, remove, editParent};
                    }}
                    onDeleted={(id) => {
                        if (selectedItemId === id) closePanel();
                    }}
                />
            </article>
        {/each}
    </section>

    {#if selectedElement}
        <aside class="info-panel">
            <button class="close" aria-label="Informationspanel schließen" onclick={closePanel}>×</button>
            <h2>{selectedElement.name}</h2>
            <p class="element-type">{selectedElement.type}</p>

            {#if selectedElement.description}
                <h3>Beschreibung</h3>
                <p>{selectedElement.description}</p>
            {/if}

            {#if selectedElement.type === "Skill" && selectedElement.stateName}
                <p><strong>Entwicklungsstatus:</strong> {selectedElement.stateName}</p>
            {/if}

            {#if selectedElement.type === "Goal"}
                <p><strong>Abgeschlossen:</strong> {selectedElement.isCompleted ? "Ja" : "Nein"}</p>
                {#if selectedElement.dueAt}
                    <p><strong>Fällig:</strong> {new Date(selectedElement.dueAt).toLocaleDateString("de-DE")}</p>
                {/if}
                {#if selectedElement.completedAt}
                    <p><strong>Abgeschlossen am:</strong> {new Date(selectedElement.completedAt).toLocaleDateString("de-DE")}</p>
                {/if}
            {/if}

            {#if selectedElement.type === "Task"}
                {#if selectedElement.stateName}
                    <p><strong>Status:</strong> {selectedElement.stateName}</p>
                {/if}
                {#if selectedElement.groupName}
                    <p><strong>Gruppe:</strong> {selectedElement.groupName}</p>
                {/if}
                {#if selectedElement.completedAt}
                    <p><strong>Abgeschlossen am:</strong> {new Date(selectedElement.completedAt).toLocaleDateString("de-DE")}</p>
                {/if}
            {/if}

            {#if selectedElement.parentName}
                <p><strong>Parent:</strong> {selectedElement.parentName}</p>
            {/if}

            <div class="panel-actions">
                <button onclick={selectedElement.edit}>Bearbeiten</button>
                <button onclick={selectedElement.remove}>Löschen</button>
                {#if selectedElement.editParent}
                    <button onclick={selectedElement.editParent}>Parent</button>
                {/if}
            </div>
        </aside>
    {/if}
</div>

<dialog bind:this={dialog}>
    <form onsubmit={(event) => {
        event.preventDefault();
        void submit();
    }}>
        <h3>{editingArea ? "Bereich bearbeiten" : "Neuer Bereich"}</h3>

        <label for="area-name">Name</label>
        <input id="area-name" bind:value={name} required />

        <label for="area-description">Beschreibung</label>
        <textarea id="area-description" bind:value={description}></textarea>

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
    .development-view {
        display: grid;
        grid-template-columns: minmax(0, 1fr) 20rem;
        gap: 2rem;
    }

    .tree-pane {
        min-width: 0;
    }

    article {
        padding: 1rem 0;
        border-bottom: 1px solid #ddd;
    }

    .area-heading {
        margin-bottom: 0.5rem;
    }

    .area-name {
        width: 100%;
        padding: 0.6rem 0.75rem;
        border: 0;
        border-radius: 0.25rem;
        background: #f2f2f2;
        text-align: left;
        font-size: 1.05rem;
        font-weight: bold;
        cursor: pointer;
    }

    .area-name.selected {
        background: #e8e8e8;
    }

    .info-panel {
        position: sticky;
        top: 2rem;
        align-self: start;
        min-height: 12rem;
        padding: 0 0 1rem 1.5rem;
        border-left: 1px solid #dddddd;
    }

    .info-panel h2 {
        margin: 0 2rem 0.25rem 0;
    }

    .info-panel h3 {
        margin-bottom: 0.25rem;
        font-size: 1rem;
    }

    .info-panel p {
        margin: 0.5rem 0;
    }

    .element-type {
        color: #666666;
    }

    .close {
        float: right;
        border: 0;
        background: transparent;
        font-size: 1.25rem;
        cursor: pointer;
    }

    .panel-actions {
        display: flex;
        gap: 0.5rem;
        margin-top: 1.25rem;
    }

    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>
