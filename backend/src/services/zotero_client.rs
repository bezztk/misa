use std::time::Duration;

use serde_json::Value;

use crate::api::error::{ApiError, ApiResult};

pub async fn translate_web(url: &str) -> ApiResult<Value> {
    let response = reqwest::Client::new()
        .post("http://127.0.0.1:1969/web?single=1")
        .header(reqwest::header::CONTENT_TYPE, "text/plain")
        .timeout(Duration::from_secs(45))
        .body(url.to_owned())
        .send()
        .await
        .map_err(|_| ApiError::BadGateway)?;
    if !response.status().is_success() {
        return Err(ApiError::BadGateway);
    }
    let bytes = response.bytes().await.map_err(|_| ApiError::BadGateway)?;
    serde_json::from_slice(&bytes).map_err(|_| ApiError::BadGateway)
}
