pub fn valid_name(name: &str, max_length: usize) -> bool {
    !name.trim().is_empty() && name.chars().count() <= max_length
}
