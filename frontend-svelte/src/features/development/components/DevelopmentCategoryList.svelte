<script lang="ts">
    import {onMount} from "svelte";
    import {
        createDevelopmentCategory,
        deleteDevelopmentCategory,
        getDevelopmentCategories,
        updateDevelopmentCategory,
    } from "../../../lib/api/developmentCategories";
    import type {DevelopmentCategory} from "../../../lib/models/developmentCategory";

    let {areaId}: {areaId: number} = $props();

    let categories: DevelopmentCategory[] = $state([]);
    let editingCategory: DevelopmentCategory | null = $state(null);
    let name = $state("");
    let error = $state("");
    let formError = $state("");
    let pending = $state(false);
    let dialog: HTMLDialogElement;

    async function loadCategories() {
        try {
            error = "";
            categories = (await getDevelopmentCategories())
                .filter(category => category.area_id === areaId);
        } catch (err) {
            error = String(err);
        }
    }

    function openCreate() {
        error = "";
        formError = "";
        editingCategory = null;
        name = "";
        dialog.showModal();
    }

    function openEdit(category: DevelopmentCategory) {
        error = "";
        formError = "";
        editingCategory = category;
        name = category.name;
        dialog.showModal();
    }

    async function submit() {
        if (pending) return;

        try {
            formError = "";
            pending = true;

            if (editingCategory) {
                await updateDevelopmentCategory(editingCategory.item_id, {name});
            } else {
                await createDevelopmentCategory({area_id: areaId, name});
            }

            dialog.close();
            await loadCategories();
        } catch (err) {
            formError = String(err);
        } finally {
            pending = false;
        }
    }

    async function remove(category: DevelopmentCategory) {
        if (pending || !confirm(`Kategorie "${category.name}" löschen?`)) return;

        try {
            error = "";
            pending = true;
            await deleteDevelopmentCategory(category.item_id);
            await loadCategories();
        } catch (err) {
            error = String(err);
        } finally {
            pending = false;
        }
    }

    onMount(() => {
        void loadCategories();
    });
</script>

<section>
    <h3>Kategorien</h3>
    <button onclick={openCreate} disabled={pending}>Neue Kategorie</button>

    {#if error}
        <p>{error}</p>
    {/if}

    {#each categories as category (category.item_id)}
        <article>
            <strong>{category.name}</strong>
            <button onclick={() => openEdit(category)} disabled={pending}>Bearbeiten</button>
            <button onclick={() => remove(category)} disabled={pending}>Löschen</button>
        </article>
    {/each}
</section>

<dialog bind:this={dialog}>
    <form onsubmit={(event) => {
        event.preventDefault();
        void submit();
    }}>
        <h3>{editingCategory ? "Kategorie bearbeiten" : "Neue Kategorie"}</h3>

        <label for="category-name">Name</label>
        <input id="category-name" bind:value={name} required />

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
    article {
        padding: 1rem 0;
        border-bottom: 1px solid #ddd;
    }

    article button {
        margin-left: 0.5rem;
    }

    form {
        display: flex;
        flex-direction: column;
        gap: 0.5rem;
    }
</style>
