
# REAL ADAPTIVE PURGE (RAP)

[MÔ TẢ](#mô-tả-sơ-bộ)
 • [QUY TRÌNH](#quy-trình-hoạt-động)
 • [YÊU CẦU CẦN THIẾT](#yêu-cầu-cần-thiết)
 • [CÀI ĐẶT](#hướng-dẫn-cài-đặt)
 • [SỬ DỤNG](#hướng-dẫn-sử-dụng)
 • [LỖI THƯỜNG GẶP](#các-lỗi-thường-gặp)
 • [CREDITS](#credits)

## MÔ TẢ SƠ BỘ

* ~~Macro cho [Klipper](https://github.com/Klipper3d/klipper), dựa trên [KAMP](https://github.com/kyleisah/Klipper-Adaptive-Meshing-Purging) để tạo `Purge-Line` tùy biến theo đối tượng in.~~

* Python script đọc file gcode mà slicer tạo ra (hỗ trợ cho [OrcaSlicer](https://github.com/SoftFever/OrcaSlicer) và [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer)) để tạo đường Purge-Line gần với điểm đầu tiên mà file in sẽ thực sự bắt đầu.

* Script hình thành dựa trên ý tưởng của [KAMP](https://github.com/kyleisah/Klipper-Adaptive-Meshing-Purging) macro, `Purge-Line` sẽ được tính toán để đặt gần nhất với đối tượng in. Điều này sẽ giúp giảm đi khoảng cách đầu in phải di chuyển từ lúc Purge đến lúc bắt đầu in. Giúp giảm tình trạng tơ (stringing) sau khi purge (nhất là với các loại nhựa có độ nhớt cao (high viscosity) như là PETG)
* Với phiên bản trước đây script sẽ tìm điểm mà máy sẽ bắt đầu in và chuyển qua cùng file gcode. Và trên máy in (chạy [Klipper](https://github.com/Klipper3d/klipper)) một macro tương ứng sẽ đọc giá trị đó và tính toán điểm thích hợp nhất và in `Purge-Line`.

    Nhưng do một số hạn chế về việc xử lý của macro của Klipper cũng như làm quá trình xử lý phức tạp hơn (đòi hỏi cả slicer và cả máy in cùng phải ăn khớp với nhau). Nên phiên bản mới (r2) sẽ chuyển hoàn toàn việc tính toán này sang bằng python script.

* Việc tính toán hoàn toàn bằng script ngoài việc cài đặt đơn giản hơn, dễ dàng tùy biến các thông số như là khoảng cách, độ dài, lưu lượng.., ngoài ra thì việc file gửi đến máy in chỉ là các câu lệnh đơn giản thì nên là trên lý thuyết sẽ không quan trọng máy in chạy phần mềm gì ([Klipper](https://github.com/Klipper3d/klipper) hoặc [Marlin](https://github.com/MarlinFirmware/Marlin)) vãn có thể sử dụng được.

    > (Tuy nhiên với [Marlin](https://github.com/MarlinFirmware/Marlin) có một số cần lưu ý sẽ được nói rõ hơn ở các mục phía sau)

## QUY TRÌNH HOẠT ĐỘNG

### QUY TRÌNH SỬ DỤNG SLICER

Phần mềm Slicer sẽ vẫn hoạt động như bình thường, trước khi file được ghi ra file (hoặc gửi qua mạng lên máy in), script sẽ được tự động gọi lên và xử lý file gcode.

> **Note:** với [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer) file lưu có 2 dạng, script sẽ chỉ đọc được file có đuôi `.gcode`, nếu file xuất ra là file `.bgcode` ([Binary Gcode](https://help.prusa3d.com/article/binary-g-code_646763)) thì script sẽ không đọc được, cần phải cài đặt lại cho Slicer xuất file dạng `.gcode` thường.

### SCRIPT HOẠT ĐỘNG

Phần mềm slicer sẽ tự động gọi script *(theo đường dẫn đã thiết lập sẵn)*. Ban đầu sẽ đọc file .gcode để lấy các thông số cơ bản bao gồm.

* `First Point` - điểm đầu tiên sẽ bắt đầu in, dùng để tính đặt Purge-Line ở đâu sẽ gần điểm này nhất.

* `Travel Speed` - đây sẽ được lấy làm tốc độ tối đa để di chuyển đầu in. Tốc độ này sẽ được tự động lấy theo tốc độ mọi người thiết lập trong slicer.
* `Bed Polygon` - đây là hình dáng và kích thước của bàn in đang dùng, dùng để kiểm tra xem Purge-Line được có bị nằm ngoài bàn in hay không.
* `Firmware Retraction` - nếu máy in được thiết lập sử dụng firmware retraction, nếu có thì lệnh retract hay unretract sẽ được đổi thành G10/G11 tương ứng.

* `Object Polygon` - tập hợp các điểm tạo nên đối tượng in trên mặt phẳng 2D. Dùng để tính đặt Purge-Line ở đâu sẽ không trùng với đối tượng in.

    > Script sẽ đọc hết toàn bộ các lệnh G1 tại layer đầu tiên từ đó xác định hình dáng và kích thước đối tượng in.  
    > Việc này sẽ có đôi chút mất thời gian hơn đọc thông thông số `EXCLUDE OBJECT` (OrcaSlicer) hoặc `object info` (với PrusaSlicer), tuy nhiên thời gian tốn thêm để xử lý chỉ ở ms (mili giây) nên hầu như không có khác biệt. Và hơn nữa, cách thức xác định đối tượng bằng các lệnh in này sẽ cho kết quả chính xác hơn (sẽ bao gồm cả Brim, Skirt hay Support... nếu có).
    >
    > Hơn nữa, việc xác định đối tượng thông qua việc đọc lệnh in này sẽ tăng độ tương thích với các phần mềm khác nhau (bất kể dùng Prusa hay Orca, Marlin hay Klipper đều có thể dùng được)
    <!-- >> **Note:** Với [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer), do cách mà phần mềm xuất file .gcode mà phần Brim/Skirt sẽ bị bỏ qua (Brim/Skirt của Prusa gộp tất cả các đối tượng lại với nhau làm cho việc xác định phần nào sẽ rất khó).  
    >> Tuy nhiên, sẽ không có vấn đề gì vì phần tính toán sẽ dự trù cả việc điểm bắt đầu in không nằm trong vùng đối tượng *(có thể sẽ cần test thêm nhiều trường hợp nữa)*. -->

### TÍNH TOÁN CÁC ĐIỂM ĐỂ IN PURGE-LINE

Sau khi có được các thông số cần thiết sẽ đến bước tính toán.

#### XÁC ĐỊNH VỊ TRÍ ĐIỂM ĐẦU TIÊN

Đầu tiên là sẽ so sánh xem điểm in đầu tiên sẽ ở đâu so với đối tượng. Từ đấy tìm một điểm gần nhất với điểm đầu tiên sẽ in và nằm trên cạnh gần nhất của đối tượng in (gọi là `điểm A`).  
Từ đấy vẽ một đường thẳng đi qua hai điểm này (đường Purge-Line sẽ nằm trên đường thẳng này).

1. Điểm bắt đầu nằm trong đối tượng.

    <!-- ![img](<./IMG/Script_cal_A1.jpg>) -->

    <p align="center">
    <image src="./IMG/Script_cal_A1.jpg" width="480">
    </p>

1. Điểm bắt đầu nằm ngoài đối tượng (có thể là `Brim`, `Skirt` hoặc `Support`)

    <!-- ![img](<./IMG/Script_cal_B1.jpg>) -->

    <p align="center">
    <image src="./IMG/Script_cal_B1.jpg" width="480">
    </p>

    > **Note:** như có nói ở trên, một số trường hợp sẽ không tính `Brim` hay `Skirt` vào đối tượng in. Tuy nhiên, nếu tính cả `Brim` hoặc `Skirt`, thì cách tính toán sẽ giống như phương án 1.

#### TÍNH TOÁN VỊ TRÍ ĐƯỜNG PURGE-LINE

Trên đường thẳng mới kẻ, xác định một điểm mới (đây sẽ là điểm kết thúc cho Purge-Line).  
Tại đây có sẽ 2 trường hợp tương ứng với 2 trường hợp phía trên.

1. Điểm bắt đầu nằm trong đối tượng.  
Điểm mới sẽ được tính dựa vào điểm A, và cách A một khoảng bằng với thông số `purge_margin`

    <!-- ![img](<./IMG/Script_cal_A2.jpg>) -->

    <p align="center">
    <image src="./IMG/Script_cal_A2.jpg" width="480">
    </p>

2. Điểm bắt đầu nằm ngoài đối tượng.  
Điểm mới sẽ được tính dựa vào điểm in đầu tiên, và cũng cách một khoảng bằng với thông số `purge_margin`

    <!-- ![img](<./IMG/Script_cal_B2.jpg>) -->

    <p align="center">
    <image src="./IMG/Script_cal_B2.jpg" width="480">
    </p>

Khi tính được điểm đầu kết thúc của Purge-Line, thì vẫn trên đường thẳng đấy xác định điểm bắt đầu cho Purge-Line. Điểm này cách điểm kết thúc một đoạn bằng thông số `purge_length`

#### KIỂM TRA LẠI CÁC ĐIỂM ĐÃ TÍNH TOÁN

Sau khi các điểm đầu cuối của `Purge-Line` đã được xác định, thì sẽ đối chiều xem đường Purge này có bị tràn ra ngoài bàn in, hoặc là có trùng với các đối tượng khác hay không (trong trường hợp in nhiều đối tượng một lần).

Tùy vào từng trường hợp cụ thể, thì script sẽ tự động tính toán lại các điểm Purge cho phù hợp.

> **Note1:** hầu hết các trường hợp thì việc kiểm tra này chỉ mang tính chất an toàn, để đảm bảo file xuất ra không bị lỗi khi in, đa phần slicer sẽ tính toán điểm in phù hợp nhất rồi, nên rất ít trường hợp đường Purge bị trùng hay lỗi.  
> Tuy nhiên không có nghĩa là sẽ không có trường hợp như thế, vậy nên việc kiểm tra lại và sửa lỗi mới được thêm vào.
>
> **Note2:** mình cũng đã thử kha khá trường hợp có thể phát sinh lỗi, tuy nhiên do tính chất của việc Slicer đặt điểm in đầu hay là chọn đối tượng để in đầu tiên (mình không đi sâu vào phần code của từng Slicer nên cũng không nắm rõ) khá là random nên có thể, có thể sẽ có trường hợp lỗi chưa tính đến.

#### XÁC ĐỊNH VỊ TRÍ ĐIỂM SẼ CHÈN CODE MỚI

Khi kiểm tra xong, script sẽ tìm đến đoạn code đầu tiên có `G90` hoặc `G91` hoặc `G20` hoặc `G21`.

Đây là đoạn code thiết lập thông số mặc định cho máy in (`G90` - Absolute Positioning, `G91` - Relative Positioning, `G20` - Inch Units, `G21` - Millimeter Units). Thường sẽ được slicer mặc định được slicer đặt ngay phía sau đoạn khới động (làm nóng bàn, làm nóng đầu in... tùy thiết lập của từng máy) và trước đoạn code để bắt đầu in.

Mốc này là thời điểm sau khi kết thúc các việc như là làm nóng máy in, chùi đầu...(tùy từng thiết lập của từng máy) và trước khi bắt đầu thực sự in.

<!-- > **Note1:** điểm đặt code sẽ được chọn tại điểm dưới sau cùng của `Machine Start G-code`. -->

> **Note1:** khác với trước đây, điểm đặt code sẽ được đánh giấu bằng một đoạn mã thiết lập sẵn tại `Machine Start G-code`. Tuy nhiên để quá trình cài đặt code đơn giản hơn nên sẽ chuyển qua phương án mới này.
>
> Mặc định các phần mềm slicer sẽ tự động có ít nhất 2 trong 4 lệnh trên tại đầu của code in, tuy nhiên nếu thiết lập slicer tùy chỉnh khác đi có thể sẽ không có, hoặc vị trí bị di dời chỗ khác, nên kiểm tra lại trước để đảm bảo.

#### TẠO FILE GCODE

Lúc này script sẽ tạo đoạn mã Purge-Line và ghi vào file gcode. File sau khi được lưu sẽ có đoạn mã này.

<!-- ![img](<IMG/Orca_Export-Gcode1.jpg>) -->

<p align="center">
<image src="./IMG/Orca_Export-Gcode1.jpg" width="360">
</p>

> **Note2:** khác với phiên bản cũ của script, file gcode mới sẽ chỉ thêm lệnh macro còn lại thì sẽ dựa vào macro của Klipper tự xử lý.
>
> Ở phiên bản mới này, script sẽ xuất ra lệnh `g-code` đơn thuần, điều này sẽ giúp cho máy in chạy phần mềm gì cũng đều đọc được ([Klipper](https://github.com/Klipper3d/klipper) hoặc [Marlin](https://github.com/MarlinFirmware/Marlin) hoặc là cả [RepRap](https://github.com/Duet3D/RepRapFirmware) tuy nhiên mình chưa dùng RepRap nên cũng không chắc 100%).
>
> Và hơn nữa, vì chỉ là `g-code` thuần, khi mở lại file gcode bằng Slicer (hoặc bất kì phần mềm nào đọc được gcode), sẽ thấy được luôn đoạn Purge-Line nằm ở đâu. Rất tiện cho việc kiểm tra.

## YÊU CẦU CẦN THIẾT

1. [Python](https://www.python.org/), tải và cài đặt (nếu chưa có). Nên dùng python 3 trở lên.

    <!-- ![img](<./IMG/InstallPython.jpg>) -->

    <p align="center">
    <image src="./IMG/InstallPython.jpg" width="420">
    </p>

2. ***(Với riêng Klipper)*** Trong file `printer.cfg` -> mục `[extruder]`.  

    *"max_extrude_cross_section"* phải được set ít nhất là `5`.

    > **Note1:** Nếu chưa có mục *"max_extrude_cross_section"* thì có thể thêm vào.

    ``` yaml
    [extruder]
    max_extrude_cross_section: 5
    ```

    > **Note2:** Đây là tính năng riêng của Klipper để tránh việc đùn một lượng lớn nhựa quá trong một lần *(thường do lỗi phát sinh từ gcode)*. Tuy nhiên, mục đích của việc Purge này là để đùn nhiều nhựa nhất có thể để làm sạch đầu in, nên việc giới hạn tính năng này là không tránh khỏi.
    >
    > **Note3:** Với Marlin hay các phần mềm khác thì theo mình được biết là không có tính năng tương tự như thế này, nên việc này chỉ cần nếu máy in dùng Klipper.

### Cấu trúc cần thiết của file G-code *(tham khảo thêm)*

Script hoạt động chủ yếu bằng các thông số lấy từ file gcode được Slicer tạo ra, cho nên nếu trong file g-code không có các thông tin cần thiết thì sẽ không hoạt động được (hoặc sẽ có hoạt động nhưng sẽ không cho ra kết quả như mong muốn).

Tuy nhiên các phần mềm slicer hiện tại (test với [OrcaSlicer](https://github.com/SoftFever/OrcaSlicer) 2.2.0 và [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer) 2.9.1) đều mặc định sẽ xuất các thông số về file in, các thông số in, máy in, nhựa in đi cùng với file g-code tạo ra. Nên nếu không có thay đổi trong các thiết lập của Slicer thì các mục dưới đây sẽ tự mặc định là có sẵn.

Các thông số cần có trong file g-code (mở file .gcode bất kì, `Ctrl+F` để tìm).

* `LAYER_CHANGE` - đánh giấu khi thay đổi layer, giúp xác định điểm bắt đầu và kết thúc của layer đầu tiên
* `travel_speed` - giúp tìm được tốc độ tối đa mà máy in có thể di chuyển, con số này không thực sự quá cần thiết, nếu gcode gửi lệnh di chuyển quá tốc độ tối đa cài trên máy, thì máy cũng sẽ tự giảm xuống.
* `use_firmware_retraction` - nếu máy cài `firmware retraction` thì sẽ dùng thông số cài trên máy.
* `bed_shape` - dùng để định hình bàn in, kiểm tra Purge-Line có bị tràn ra ngoài.
* `G20, G21, G90, G91` - dùng để định vị điểm trước khi máy sẽ bắt đầu in, kiểm tra file gcode sẽ có ít nhất 1 trong 4 lệnh trên phía sau đoạn lệnh khởi động của máy và trước phần in (trước đoạn `LAYER_CHANGE` đầu tiên).
* `TYPE` - để xác định được phân code nào sẽ in gì, giúp xác định được đối tượng in

    > Các loại TYPE thường có sẽ là
    > * `Outer wall` - đây là lớp tường bên ngoài với OrcaSlicer
    > * `External perimeter` - đây cũng là lớp tường ngoài nhưng là với PrusaSlicer
    > * `Brim` - loại brim (thường khi dùng mới có)
    > * `Skirt` - loại Skirt (thường khi dùng mới có)
    > * `Support` - các loại liên quan đển support (support, raft... thường khi dùng mới có)

## HƯỚNG DẪN CÀI ĐẶT

1. Đầu tiên tải file `RAP.py`, đây là file script sẽ dùng để xử lý gcode.

2. Thiết lập cho phần mềm Slicer.

### Cài đặt cho [OrcaSlicer](https://github.com/SoftFever/OrcaSlicer)

* Ở bảng chỉnh thông số in, vào tab `OTHER` (tab thường dùng để chỉnh Skirt hay Brim)

* Phía dưới tại mục `Post-Processing Scripts`, điền vào đường dẫn tới file `Python.exe` và script `RAP.py`

    ```none
    "DUONG-DAN-DEN-PYTHON\python.exe" "DUONG-DAN-DEN-SCRIPT\RAP.py";
    ```

    <!-- ![orca](<./IMG/Orca_Script1.jpg>) -->

    <p align="center">
    <image src="./IMG/Orca_Script1.jpg" width="600">
    </p>

    > **Note1:** chú ý phải có dấu (") hai đầu và phải có đoạn cách giữa hai đường dẫn để phần mềm có thể đọc được.  
    > `DUONG-DAN-DEN-PYTHON` là nơi python cài trên máy.
    >> Để xác định đường dẫn của `python.exe` (nếu chưa biết), có thể sử dụng `command Prompt`.  
    >> ***(windows)*** mở `command Prompt` (Start -> cmd -> Enter) hoặc (Win + R -> cmd -> Enter).  
    >> Nhập vào:
    >>
    >> ```cmd
    >> where python
    >> ```
    >>
    >> Các đường dẫn về nơi `python.exe` sau khi cài nằm ở đâu sẽ được liệt kê ra, chỉ copy một đường dẫn và dán vào phần mềm slicer *(thường chọn đường dẫn đầu tiên là được)*
    >
    > `DUONG-DAN-DEN-SCRIPT` là nơi đặt file script trên máy. Đường dẫn này sau khi thiết lập thì file không được di chuyển đi chỗ khác (nếu di chuyển phải thiết lập lại), có thể để cùng thư mục với phần mềm Slicer để không xóa nhầm hoặc lỡ di chuyển đi.  

<!-- * Mở bảng thiết lập cho máy in và vào tab `Machine G-code`.

    Ở mục `Machine start G-code` thêm dòng `; Adaptive Purge START` vào dưới cùng. Đây sẽ như là một điểm mốc để script chèn code cho đoạn Purge và file gcode.

    ![img](<./IMG/Orca_Script2.jpg>)

    > **Note2:** Trong hình mình chỉ có mỗi PRINT_START nhưng với những máy có nhiều code hơn thì cũng cho dòng `; Adaptive Purge START` vào dưới cùng.
    >
    > **Note3:** Nên chú ý có dấu `;` phía trước để máy in sẽ không đọc mà bỏ qua.  
    `Script` sẽ dùng đoạn này như mốc để xác định đâu là đoạn code sau giai đoạn chuẩn bị của máy in (làm nóng, home, làm sạch đầu in...) và trước khi phần in thực sự bắt đầu.
    >
    >> Mình đã thử tìm một số điểm làm mốc khác để đơn giản hơn công đoạn cài đặt nhưng phần đầu trước khi in này sẽ thay đổi nhiều tùy vào nhiều thết lập trong slicer cũng như máy in dùng Klipper hay Marlin.  
    >> Nên để chắc chắn chính xác đoạn cần chèn thên script thì tạo một mốc đặc trưng như thế này sẽ là cách tối ưu. -->

* SAVE lại để dùng sau này.

### Cài đặt cho [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer)

*Cơ bản hoàn toàn giống với Orca Slicer, chỉ khác là giao diện của hai phần mềm khác nhau.*

* Mở tab `Print Settings`
* Mục `Output Options` -> bảng `Post-processing scripts`

    <!-- ![img](<./IMG/Prusa_Script1.jpg>) -->

    <p align="center">
    <image src="./IMG/Prusa_Script1.jpg" width="600">
    </p>

* Điền vào đường dẫn tới file `Python.exe` và script `RAP.py`:

    ```none
    "DUONG-DAN-DEN-PYTHON\python.exe" "DUONG-DAN-DEN-SCRIPT\RAP.py";
    ```

    > **Note1:** chú ý phải có dấu (") hai đầu và phải có đoạn cách giữa hai đường dẫn để phần mềm có thể đọc được.  
    > `DUONG-DAN-DEN-PYTHON` là nơi python cài trên máy.
    >> Để xác định đường dẫn của `python.exe` (nếu chưa biết), có thể sử dụng `command Prompt`.  
    >> ***(windows)*** mở `command Prompt` (Start -> cmd -> Enter) hoặc (Win + R -> cmd -> Enter).  
    >> Nhập vào:
    >>
    >> ```cmd
    >> where python
    >> ```
    >>
    >> Các đường dẫn về nơi `python.exe` sau khi cài nằm ở đâu sẽ được liệt kê ra, chỉ copy một đường dẫn và dán vào phần mềm slicer *(thường chọn đường dẫn đầu tiên là được)*
    >
    > `DUONG-DAN-DEN-SCRIPT` là nơi đặt file script trên máy. Đường dẫn này sau khi thiết lập thì file không được di chuyển đi chỗ khác (nếu di chuyển phải thiết lập lại), có thể để cùng thư mục với phần mềm Slicer để không xóa nhầm hoặc lỡ di chuyển đi.  

<!-- * Mở tab `Printers` để mở bảng thiết lập cho máy in.

    Ở mục `Custom G-code` -> `Start G-code`  
    Thêm dòng `; Adaptive Purge START` vào dưới cùng. Đây sẽ như là một điểm mốc để script chèn code cho đoạn Purge và file gcode.

    ![img](<./IMG/Prusa_Script2.jpg>)

    > **Note2:** Trong hình mình chỉ có mỗi PRINT_START nhưng với những máy có nhiều code hơn thì cũng cho dòng `; Adaptive Purge START` vào dưới cùng.
    >
    > **Note3:** Nên chú ý có dấu `;` phía trước để máy in sẽ không đọc mà bỏ qua.  
    `Script` sẽ dùng đoạn này như mốc để xác định đâu là đoạn code sau giai đoạn chuẩn bị của máy in (làm nóng, home, làm sạch đầu in...) và trước khi phần in thực sự bắt đầu.
    >
    >> Mình đã thử tìm một số điểm làm mốc khác để đơn giản hơn công đoạn cài đặt nhưng phần đầu trước khi in này sẽ thay đổi nhiều tùy vào nhiều thết lập trong slicer cũng như máy in dùng Klipper hay Marlin.  
    >> Nên để chắc chắn chính xác đoạn cần chèn thên script thì tạo một mốc đặc trưng như thế này sẽ là cách tối ưu. -->

* SAVE lại để dùng sau này.

## HƯỚNG DẪN SỬ DỤNG

* Sau quá trình cài đặt thì quá trình sử dụng không có khác gì so với trước (vì toàn bộ sẽ tự động chạy).

* Và quá trình xử lý file sẽ rất nhanh nên sẽ không biết được file đã được xử lý hay chưa (một số trường hợp nếu có lỗi phát sinh cũng không có thông báo). Nên sau khi slice thử file, lưu file .gcode (thay vì chọn `Print` thì chọn `Export G-Code file`) và mở lại bằng slicer.

    > Với [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer), kéo thả file `.gcode` vào lại phần mềm Slicer (hoặc mở thư mục cài, thì sẽ có phần mềm `prusa-gcodeviewer.exe`, mở lên và load file `.gcode` vừa tạo).
    >
    > Với [OrcaSlicer](https://github.com/SoftFever/OrcaSlicer) thì chức năng xem file được tích hợp cùng nhau, nên chỉ có thể kéo thả file `.gcode` vào chứ không có chức năng riêng.  
    >> **Note:** với Prusa sẽ tự mở một cửa sổ mới, nhưng Orca khi kéo thả `.gcode` vào thì các file đang làm việc sẽ bị mất thay vào đó là file `.gcode` nên tốt nhất là nên mở một cửa sổ mới rồi hẵng kéo thả vào.  
    >> *(Điều lạ hơn là ở Orca, chỉ có thể kéo thả vào chứ không có mục nào cho chọn để mở file đuôi .gcode cả)*  
    >> Và với file .gcode không phải do Orca xuất ra thì không mở được. Với điểm này, +1 cho Pursa Team.

    Khi mở file `.gcode` ra thì đường sẽ thấy đường purge được tạo ra như hình.

    <!-- ![img](<IMG/Orca_Export-Gcode1.jpg>) -->

    <p align="center">
    <image src="./IMG/Orca_Export-Gcode1.jpg" width="480">
    </p>

* Trên Web UI (ví dụ như là [Mainsail](https://github.com/mainsail-crew/mainsail)) cũng sẽ cho xem trước file gcode trước khi in, sau khi upload lên máy in có thể mở trên Web UI để xem trực tiếp.

### Các thông số của đường Purge-Line

Các thông số này đều được thiết lập sẵn, tuy nhiên có thể tùy chỉnh.

| TÊN GỌI THÔNG SỐ   | GIÁ TRỊ MẶC ĐỊNH | GIẢI THÍCH Ý NGHĨA    |
| ---                | ---              | ---                   |
| `-verbose`         | False            | Xuất thông tin khi chạy script. Thông số này không có tác dụng khi chạy script trong Slicer, nếu chỉ dùng ở Silcer thì bỏ qua thông số này. |
| `-purge_height`    | 0.5mm            | Vị trí đầu in khi in Purge-Line, hoặc có thể hiểu là cao độ Z. |
| `-tip_distance`    | 1.0mm            | Khoảng cách giữa đầu nhựa và đầu in trước khi Purge. Thường sẽ thiết lập bằng khoảng cách sợi nhựa sẽ rút lại lúc kết thúc file in trước. Thông số này không quá quan trọng, có thể để mặt định cũng không thay đổi nhiều. |
| `-purge_margin`    | 5mm              | khoảng cách giữa điểm in Purge-Line và đối tượng in. Nếu thấy đoạn Purge hơi gần đối tượng có thể tăng khoảng cách này. |
| `-purge_length`    | 10mm             | Chiều dài của đoạn Purge-Line. Thay đổi nếu muốn đoạn Purge dài ra hoặc ngắn lại. |
| `-purge_amount`    | 10mm             | Chiều dài sợi nhựa sẽ dùng để Purge. Dài hơn thì đầu in sẽ sạch hơn (Purge nhiều nhựa hơn) đồng thời cũng tốn nhựa hơn. |
| `-flow_rate`       | 12mm3            | Lưu lượng khi purge. Nên chọn mức nhiều nhất có thể mà đầu in máy đang dùng có thể xuất được. vd: 12mm3 là giá trị tối đa cho đầu in E3D V6 theo nhà sản xuất. |

> **Note:** Khi nhập tùy chỉnh thì chỉ cần giá trị chứ không cần đơn vị.  
Ví dụ, `purge_margin` muốn chuyển thành 20mm thì chỉ cần `-purge_margin 20` nếu nhập thành `-purge_margin 20mm` hay là `-purge_margin 20 mm` thì script sẽ không đọc được.

### Tùy Chỉnh thông số có 2 phương pháp

1. Điều chỉnh bằng dòng lệnh trực tiếp trong Slicer.

    Các thông số có thể thay tùy chỉnh cũng tương ứng với khi chỉnh sửa trực tiếp tại script.  
    Trong bảng `Post-Processing Scripts` (khi nhập cài đặt script), thêm giá trị muốn thay đổi và giá trị thay đổi phía sau script là được.

    Ví dụ: Khi muốn thay đổi `purge_margin` thành 20mm, tại ô `Post-Processing Scripts` thông số sẽ được nhập vào như sau:

    ```py
    "XXX\python.exe" "YYY\RAP.py" -purge_margin 20;
    ```

    Hoặc khi muốn thay đổi cả `purge_margin` và `purge_amount`:

    ```py
    "XXX\python.exe" "YYY\RAP.py" -purge_margin 20 -purge_amount 20;
    ```

    > **Note:** Thứ tự các thông số không quan trọng, chỉ cần đảm bảo đúng giá trị và cấu trúc của từng thông số *(tên thông số đến 'khoảng trống' đến giá trị)* và lưu ý có dấu `-` ở phía trước.

2. Điều chỉnh trực tiếp trong script `RAP.py`

    * Mở file script `RAP.py` bằng một phần mềm edit text bất kì, hoặc tốt hơn nên mở bằng phần mềm chuyên edit code (ví dụ [VSCode](https://code.visualstudio.com/)).

    * Khi kéo xuống phía dưới cùng của script sẽ có đoạn như hình.

        ![img](<./IMG/Script_Code1.jpg>)

        Ở đây sẽ gồm các thông số như bảng trên, chỉnh sửa ở đoạn `default=??` tương ứng cho thông số muốn thay đổi và save lại.

# CÁC LỖI THƯỜNG GẶP

* **Error code: 2**

    Thường khi gặp lỗi này thì là do đường dẫn đến script không đúng, có thể thiếu dấu `"` ở hai đầu, hoặc thiếu đấu cách giữa python với script, kiểm tra lại phần thiết lập Script cho Slicer.

* **File xuất ra 0kb**

    Khi slicer xuất file không có lỗi gì, nhưng file xuất lại không có gì bên trong (dung lượng file là 0kb)  
    Khả năng cao là do script không tìm được điểm để ghi chèn code vào file in. Xem cụ thể hơn về mốc để script chèn code mới ở [đây](#xác-định-vị-trí-điểm-sẽ-chèn-code-mới) hoặc về cấu trúc file gcode để script hoạt động tại [đây](#cấu-trúc-cần-thiết-của-file-g-code-tham-khảo-thêm).
    > Script trên cơ bản gọi là chèn phần code mới vào file .gcode mà slicer tạo ra nhưng thực tế là tạo một file code mới và ghi đè lên.  
    > Do vậy khi thiếu điểm mốc thì script sẽ không tìm được đoạn để ghi gcode -> không có gcode sau khi sửa -> ghi đè một file không có gì vào file slicer xuất ra -> file trống không.

* **Các lỗi khác**

    Thường nếu script không khoạt động như mong muốn, có thể do cấu trúc file gcode không được giống như mặc định Slicer xuất ra (có thể do tùy chỉnh từng dòng máy, hoặc phiên bản Slicer khác quá nhiều). Kiểm tra lại cấu trúc file gcode để script hoạt động tại [đây](#cấu-trúc-cần-thiết-của-file-g-code-tham-khảo-thêm)

# CREDITS

* [KAMP](https://github.com/kyleisah/Klipper-Adaptive-Meshing-Purging) - phát triển trên ý tưởng của KAMP.
* [Klipper](https://github.com/Klipper3d/klipper)
* [OrcaSlicer](https://github.com/SoftFever/OrcaSlicer) - đã test trên OrcaSlicer 2.2.0
* [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer) - đã test trên PrusaSlicer 2.9.1
