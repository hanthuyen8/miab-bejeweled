# Candy Crush Lightning Strike Effect
Url: https://forum.defold.com/t/candy-crush-lightning-strike-effect/67561

# Question:
How can i make lightning effect like link below? Shader or animation?

![candy-crush-screen-shot](candy-crush-screen-shot.png)

# Answer from Defold's staff:
One way is with sprites only. You have lightning segments (with some random variation) and soft circular glow textures which you draw together.

Think about this image where every color is a random variation. Then with start/end segments you would have the edge taper off.

![demo](demo.png)

![segments](segments.png)

For connecting / generating the segments you have a start and end position and pick some random variation for the connection points of the segments toward the final position with the final positioning exactly pointing at the end position.

# Reply to Defold's staff:
Thank you for the answer.

You mean snake style. Join head, body, tale in the tile?

https://github.com/HelloWorldSweden/Defold-Snake


# Phân tích chi tiết câu trả lời của Defold's staff

Bóc tách lại kỹ thuật "segment chain" mà staff mô tả thành các bước cụ thể:

1. **Xác định điểm đầu/cuối**: ví dụ 2 viên kẹo bị nối bởi lightning — có 1 start position và 1 end position cố định.
2. **Chia đoạn (segments)**: chia đường thẳng nối 2 điểm thành N đoạn nhỏ (N tùy độ dài/độ chi tiết mong muốn).
3. **Random jitter theo phương vuông góc**: với mỗi điểm nối giữa 2 đoạn liên tiếp, offset ngẫu nhiên theo phương vuông góc (perpendicular) với hướng chính của tia sét — đây chính là thứ tạo ra dáng "zigzag" đặc trưng, chứ không phải random hoàn toàn theo mọi hướng. Random toàn hướng sẽ ra hình răng cưa lộn xộn, không giống sét.
4. **Điểm cuối cố định**: dù các điểm giữa có random, điểm cuối luôn được ép về đúng vị trí end position — nghĩa là đoạn cuối cùng luôn "chỉ thẳng" vào đích, không random. Đây là chi tiết dễ bị bỏ sót nhưng quan trọng để tia sét trông có chủ đích thay vì trôi tự do.
5. **Vẽ từng đoạn bằng sprite thon dài**: mỗi segment là 1 sprite (quad dài, hẹp), xoay theo đúng góc của đoạn đó (tính từ 2 điểm đầu-cuối segment). Đầu và đuôi toàn bộ chain dùng sprite/texture riêng có texture taper (thon nhỏ dần) để tia sét không bị cắt cụt ở 2 đầu.
6. **Glow texture**: chồng thêm texture tròn mềm (soft circular glow, thường additive blend) tại các điểm nối giữa 2 segment — vừa che mối nối (segment thường không nối liền mạch 100% do góc xoay khác nhau), vừa tạo cảm giác phát sáng.
7. **Random variation về màu**: staff nhấn mạnh "every color is a random variation" trong ảnh demo — tức mỗi segment có thể lệch màu/độ sáng nhẹ so với segment kế bên, tạo cảm giác điện chớp không đều thay vì 1 khối màu đồng nhất.

**Nhận xét**: đây thực chất là một dạng đơn giản hóa của "midpoint displacement" (kỹ thuật sinh địa hình fractal / đường bờ biển) — áp dụng đệ quy 1 cấp (chia đều N đoạn + jitter) thay vì đệ quy nhiều cấp. Vì lightning bolt trong game chỉ cần "đủ giống" trong một khung hình nhỏ và thời gian ngắn, không cần độ chi tiết fractal thật sự, nên cách làm đơn giản này là đánh đổi hợp lý giữa chất lượng thị giác và chi phí tính toán — khớp với nhận định ở phần so sánh performance bên dưới rằng cách này rẻ và dễ throttle.

Điểm chưa rõ trong câu trả lời gốc: staff không nói rõ N (số segment) nên chọn bao nhiêu, hay tần suất regenerate random offset (mỗi frame hay mỗi vài frame) — đây là 2 tham số cần tự thử nghiệm/tune khi implement.

# Other question and answer on stackoverflow (maybe not related to game):

https://gamedev.stackexchange.com/questions/71397/how-can-i-generate-a-lightning-bolt-effect

# Research thêm (2026-07-23)

Đã search thử xem Slay the Spire / Vampire Survivors có tài liệu chính thức nào (dev blog, GDC talk) nói về cách họ render lightning không — **không tìm thấy**. Không có nguồn nào xác nhận 2 game này dùng kỹ thuật cụ thể nào, nên bỏ qua giả định trước đó.

Tìm được thêm các nguồn kỹ thuật chung (không riêng game nào):
- https://jettelly.com/blog/building-a-lightning-shader-in-unity-using-a-single-material — dùng ray marching + Posterize node để bẻ gradient mượt thành hình răng cưa giống sét thật, đẩy màu vượt ngưỡng HDR (>1.0) để trigger bloom tự động ở viền.
- https://github.com/keijiro/SpektrLightning — Unity line/shader renderer vẽ bolt giữa 2 điểm, khuyến nghị kết hợp HDR bloom.
- https://github.com/nullsoftware/UnityLightning

## So sánh performance: sprite-based vs shader-based

**Sprite-based (segment + sprite, theo Defold forum ở trên) — thường nhanh hơn, hợp cho mobile/2D:**
- Chỉ là quad + texture bình thường → tận dụng batching/instancing chung với các sprite khác.
- Không cần shader riêng, không cần render pass phụ (không HDR buffer, không bloom pass).
- Chi phí chủ yếu ở CPU (tính vị trí segment) — rẻ, dễ throttle (update mỗi 2-4 frame thay vì mỗi frame để có hiệu ứng rung giật của điện).
- Cần chú ý gộp atlas/material để tránh tăng draw call khi nhiều segment.

**Shader-based (ray marching / posterize + bloom):**
- Ray marching trong fragment shader tốn GPU hơn đáng kể, đặc biệt trên GPU mobile (fill-rate bound).
- Cần thêm bloom/HDR post-process pass → thêm 1 full-screen pass, tốn bandwidth.
- Đổi lại cho hiệu ứng mượt/đẹp hơn ở độ phân giải cao, ít lặp pattern.

**Kết luận cho project này**: codebase đang ưu tiên giảm draw call (xem commit `optimize drawcall`, `optimize build size`), nên hướng sprite-based (dùng chung atlas/material với sprite khác) hợp hơn shader/bloom pass riêng.