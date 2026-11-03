use crate::models::item_type::ItemType;
use sqlx::{PgPool, Postgres, Transaction};

pub async fn create_item(
    tx: &mut Transaction<'_, Postgres>,
    item_type: ItemType,
) -> Result<i32, sqlx::Error> {
    sqlx::query_scalar("INSERT INTO items (type) VALUES ($1) RETURNING id::INTEGER")
        .bind(item_type as i32)
        .fetch_one(&mut **tx)
        .await
}

pub async fn update_item(
    tx: &mut Transaction<'_, Postgres>,
    item_id: i64,
) -> Result<(), sqlx::Error> {
    sqlx::query(
        "UPDATE items
         SET updated_at = NOW(), version = version + 1
         WHERE id = $1",
    )
    .bind(item_id)
    .execute(&mut **tx)
    .await?;

    Ok(())
}

#[derive(sqlx::FromRow)]
struct HierarchyItem {
    item_type: i32,
    area_id: Option<i32>,
    group_id: Option<i32>,
}

pub enum UpdateParentResult {
    Updated,
    NotFound,
    Invalid,
}

const ITEM_FOR_HIERARCHY: &str =
    "SELECT
        i.type AS item_type,
        COALESCE(t.area_id, c.area_id, g.area_id, s.area_id) AS area_id,
        t.group_id
     FROM items i
     LEFT JOIN tasks t ON t.item_id = i.id
     LEFT JOIN development_categories c ON c.item_id = i.id
     LEFT JOIN goals g ON g.item_id = i.id
     LEFT JOIN skills s ON s.item_id = i.id
     WHERE i.id = $1";

pub async fn update_item_parent(
    pool: &PgPool,
    child_id: i32,
    parent_id: Option<i32>,
) -> Result<UpdateParentResult, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let child = sqlx::query_as::<_, HierarchyItem>(ITEM_FOR_HIERARCHY)
        .bind(child_id)
        .fetch_optional(&mut *tx)
        .await?;

    // Child not found
    let Some(child) = child else {
        return Ok(UpdateParentResult::NotFound);
    };

    // Remove Parent
    let Some(parent_id) = parent_id else {
        sqlx::query("DELETE FROM item_hierarchy WHERE child_id = $1")
            .bind(child_id)
            .execute(&mut *tx)
            .await?;

        update_item(&mut tx, i64::from(child_id)).await?;

        tx.commit().await?;
        return Ok(UpdateParentResult::Updated);
    };

    // Child = Parent
    if child_id == parent_id {
        return Ok(UpdateParentResult::Invalid);
    }

    let parent = sqlx::query_as::<_, HierarchyItem>(ITEM_FOR_HIERARCHY)
        .bind(parent_id)
        .fetch_optional(&mut *tx)
        .await?;

    // Parent not found
    let Some(parent) = parent else {
        return Ok(UpdateParentResult::NotFound);
    };

    // Parent & Child no valid type
    let Some(child_type) = ItemType::from_id(child.item_type) else {
        return Ok(UpdateParentResult::Invalid);
    };
    let Some(parent_type) = ItemType::from_id(parent.item_type) else {
        return Ok(UpdateParentResult::Invalid);
    };

    // Parent & Child relation allowed?
    if !child_type.allowed_parent_types().contains(&parent_type)
        || child.area_id.is_none()
        || child.area_id != parent.area_id
        || (child_type == ItemType::Task && child.group_id.is_some())
        || (parent_type == ItemType::Task && parent.group_id.is_some())
    {
        return Ok(UpdateParentResult::Invalid);
    }

    let mut current_id = Some(parent_id);

    // Loop the parent.parent chain up to check if the child is in it
    while let Some(id) = current_id {
        if id == child_id {
            return Ok(UpdateParentResult::Invalid);
        }

        current_id =
            sqlx::query_scalar("SELECT parent_id FROM item_hierarchy WHERE child_id = $1")
                .bind(id)
                .fetch_optional(&mut *tx)
                .await?;
    }

    sqlx::query(
        "INSERT INTO item_hierarchy (child_id, parent_id)
         VALUES ($1, $2)
         ON CONFLICT (child_id) DO UPDATE SET parent_id = EXCLUDED.parent_id",
    )
    .bind(child_id)
    .bind(parent_id)
    .execute(&mut *tx)
    .await?;

    update_item(&mut tx, i64::from(child_id)).await?;

    tx.commit().await?;

    Ok(UpdateParentResult::Updated)
}

pub async fn update_item_archive(
    pool: &PgPool,
    item_id: i32,
    is_archived: bool,
) -> Result<bool, sqlx::Error> {
    let mut tx = pool.begin().await?;

    let result = sqlx::query("UPDATE items SET is_archived = $1 WHERE id = $2")
    .bind(is_archived)
    .bind(item_id)
    .execute(&mut *tx)
    .await?;

    if result.rows_affected() == 0 {
        tx.rollback().await?;
        return Ok(false);
    }

    update_item(&mut tx, i64::from(item_id)).await?;
    tx.commit().await?;

    Ok(true)
}
