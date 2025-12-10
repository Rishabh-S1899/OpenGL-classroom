// fragment.wgsl

@fragment
fn main() -> @location(0) vec4<f32> {
    // Return a solid red color (R, G, B, A)
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
}