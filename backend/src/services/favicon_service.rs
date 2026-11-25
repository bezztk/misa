use std::time::Duration;

use reqwest::{Client, Response, Url, header::CONTENT_TYPE};
use scraper::{Html, Selector};

use crate::models::source::Favicon;

pub async fn fetch_favicon(url: Url) -> Option<Favicon> {
    let client = Client::builder()
        .timeout(Duration::from_secs(5))
        .build()
        .ok()?;

    let mut page_url = url;
    if let Ok(page) = client.get(page_url.clone()).send().await {
        page_url = page.url().clone();
        let mime_type = content_type(&page);
        if page.status().is_success()
            && (mime_type.eq_ignore_ascii_case("text/html")
                || mime_type.eq_ignore_ascii_case("application/xhtml+xml"))
            && let Ok(html) = page.text().await
        {
            for icon_url in icon_links(&page_url, &html) {
                if let Some(icon) = download_icon(&client, icon_url).await {
                    return Some(icon);
                }
            }
        }
    }

    download_icon(&client, page_url.join("/favicon.ico").ok()?).await
}

fn icon_links(page_url: &Url, html: &str) -> Vec<Url> {
    let document = Html::parse_document(html);
    let base_selector = Selector::parse("head base[href]").unwrap();
    let icon_selector = Selector::parse("head link[rel~='icon' i][href]").unwrap();
    let base = document
        .select(&base_selector)
        .filter_map(|element| element.attr("href"))
        .next()
        .and_then(|href| page_url.join(href).ok());
    let base_url = base.as_ref().unwrap_or(page_url);

    document
        .select(&icon_selector)
        .filter_map(|element| element.attr("href"))
        .filter_map(|href| base_url.join(href).ok())
        .collect()
}

async fn download_icon(client: &Client, url: Url) -> Option<Favicon> {
    let response = client.get(url).send().await.ok()?.error_for_status().ok()?;
    let mime_type = content_type(&response).to_ascii_lowercase();
    if !mime_type.starts_with("image/")
        && mime_type != "application/octet-stream"
        && !mime_type.is_empty()
    {
        return None;
    }
    let data = response.bytes().await.ok()?.to_vec();
    if data.is_empty() {
        return None;
    }
    let mime_type = if mime_type.starts_with("image/") {
        mime_type
    } else if data.starts_with(&[0, 0, 1, 0]) {
        "image/x-icon".to_owned()
    } else {
        return None;
    };
    Some(Favicon { data, mime_type })
}

fn content_type(response: &Response) -> &str {
    response
        .headers()
        .get(CONTENT_TYPE)
        .and_then(|value| value.to_str().ok())
        .and_then(|value| value.split(';').next())
        .unwrap_or_default()
        .trim()
}
