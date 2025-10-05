; discord-asm x64 Gateway Implementation
; Platform: x86-64 (Windows/Linux/macOS)
; Assembler: NASM
; 
; This file implements the core Discord Gateway client logic in Assembly.
; It uses the C shim for WebSocket/JSON/timing operations while keeping
; the main event loop and state management in Assembly.

%ifdef WINDOWS
    %define ABI_CALL_CONV  ; Windows x64 ABI: RCX, RDX, R8, R9
    %define SHADOW_SPACE 32
%else
    %define ABI_CALL_CONV  ; SysV AMD64 ABI: RDI, RSI, RDX, RCX, R8, R9  
    %define SHADOW_SPACE 0
%endif

; External C functions from the shim
extern discord_ws_connect
extern discord_ws_send  
extern discord_ws_receive
extern discord_ws_close
extern discord_ws_free_message
extern discord_json_parse_opcode
extern discord_json_parse_hello
extern discord_json_create_identify
extern discord_json_create_heartbeat
extern discord_json_free
extern discord_time_now_ms
extern discord_sleep_ms

; Constants from opcodes.h
%define DISCORD_OP_HEARTBEAT     1
%define DISCORD_OP_IDENTIFY      2
%define DISCORD_OP_HELLO        10
%define DISCORD_OP_HEARTBEAT_ACK 11

%define DISCORD_OK               0
%define DISCORD_ERROR_TIMEOUT   -6

; Structure offsets (must match structs.h)
%define WS_MESSAGE_DATA_OFFSET   0
%define WS_MESSAGE_LENGTH_OFFSET 8
%define WS_MESSAGE_BINARY_OFFSET 16

section .data
    ; Gateway URL for Discord
    gateway_url db 'wss://gateway.discord.gg/?v=10&encoding=json', 0
    
    ; State variables
    gateway_ptr dq 0                ; Pointer to gateway structure
    heartbeat_interval dd 0         ; Heartbeat interval in ms
    last_heartbeat dq 0            ; Last heartbeat timestamp
    sequence_number dd -1          ; Current sequence number
    is_ready db 0                  ; Ready state flag
    
    ; Message buffer
    message_buffer times 4096 db 0
    
section .bss
    ; WebSocket message structure
    ws_message resb 24             ; discord_ws_message_t structure
    
section .text

; Export main gateway functions
global discord_gateway_connect
global discord_gateway_run
global discord_gateway_disconnect

;------------------------------------------------------------------------------
; discord_gateway_connect: Connect to Discord Gateway
; Input: RDI/RCX = bot token (null-terminated string)
; Output: RAX = result code (0 = success, negative = error)
;------------------------------------------------------------------------------
discord_gateway_connect:
    ; Function prologue
    push rbp
    mov rbp, rsp
    sub rsp, SHADOW_SPACE + 16     ; Shadow space + local variables
    
%ifdef WINDOWS
    mov [rbp-8], rcx               ; Save token parameter
%else
    mov [rbp-8], rdi               ; Save token parameter  
%endif
    
    ; Connect to WebSocket
%ifdef WINDOWS
    mov rcx, gateway_url           ; URL parameter
    lea rdx, [gateway_ptr]         ; Output gateway pointer
%else
    mov rdi, gateway_url           ; URL parameter
    lea rsi, [gateway_ptr]         ; Output gateway pointer
%endif
    call discord_ws_connect
    
    ; Check connection result
    test rax, rax
    jnz .connect_failed
    
    ; Connection successful
    mov rax, DISCORD_OK
    jmp .cleanup

.connect_failed:
    ; Connection failed, return error code
    ; RAX already contains the error code
    
.cleanup:
    add rsp, SHADOW_SPACE + 16
    pop rbp
    ret

;------------------------------------------------------------------------------
; discord_gateway_run: Main gateway event loop
; Input: RDI/RCX = bot token (null-terminated string)  
; Output: RAX = result code
;------------------------------------------------------------------------------
discord_gateway_run:
    ; Function prologue
    push rbp
    mov rbp, rsp
    sub rsp, SHADOW_SPACE + 32     ; Shadow space + local variables
    
%ifdef WINDOWS
    mov [rbp-8], rcx               ; Save token parameter
%else
    mov [rbp-8], rdi               ; Save token parameter
%endif
    
    ; Check if gateway is connected
    mov rax, [gateway_ptr]
    test rax, rax
    jz .not_connected
    
.main_loop:
    ; Receive message with 1 second timeout
%ifdef WINDOWS
    mov rcx, [gateway_ptr]         ; Gateway parameter
    lea rdx, [ws_message]          ; Message structure
    mov r8d, 1000                  ; 1 second timeout
%else
    mov rdi, [gateway_ptr]         ; Gateway parameter
    lea rsi, [ws_message]          ; Message structure  
    mov edx, 1000                  ; 1 second timeout
%endif
    call discord_ws_receive
    
    ; Check receive result
    cmp rax, DISCORD_ERROR_TIMEOUT
    je .check_heartbeat            ; Timeout is normal, check if heartbeat needed
    test rax, rax
    jnz .receive_error             ; Other errors are fatal
    
    ; Process received message
    call process_message
    test rax, rax
    jnz .process_error
    
    ; Free the message data
%ifdef WINDOWS
    lea rcx, [ws_message]
%else
    lea rdi, [ws_message]
%endif
    call discord_ws_free_message
    
.check_heartbeat:
    ; Check if we need to send heartbeat
    call check_and_send_heartbeat
    
    ; Continue main loop
    jmp .main_loop

.not_connected:
    mov rax, -1                    ; Gateway not connected error
    jmp .cleanup
    
.receive_error:
.process_error:
    ; Error occurred, cleanup and return error code
    jmp .cleanup

.cleanup:
    add rsp, SHADOW_SPACE + 32
    pop rbp  
    ret

;------------------------------------------------------------------------------
; process_message: Process a received WebSocket message
; Input: ws_message structure contains the message data
; Output: RAX = result code
;------------------------------------------------------------------------------
process_message:
    push rbp
    mov rbp, rsp
    sub rsp, SHADOW_SPACE + 16
    
    ; Parse opcode from JSON
    mov rax, [ws_message + WS_MESSAGE_DATA_OFFSET]
    test rax, rax
    jz .invalid_message
    
%ifdef WINDOWS
    mov rcx, rax                   ; JSON data
    lea rdx, [rbp-4]              ; Opcode output
%else
    mov rdi, rax                   ; JSON data
    lea rsi, [rbp-4]              ; Opcode output
%endif
    call discord_json_parse_opcode
    
    test rax, rax
    jnz .parse_error
    
    ; Switch on opcode
    mov eax, [rbp-4]               ; Load parsed opcode
    
    cmp eax, DISCORD_OP_HELLO
    je .handle_hello
    
    cmp eax, DISCORD_OP_HEARTBEAT_ACK  
    je .handle_heartbeat_ack
    
    ; For now, ignore other opcodes
    mov rax, DISCORD_OK
    jmp .cleanup

.handle_hello:
    call handle_hello_message
    jmp .cleanup
    
.handle_heartbeat_ack:
    call handle_heartbeat_ack_message
    jmp .cleanup

.invalid_message:
.parse_error:
    mov rax, -1                    ; Generic error
    
.cleanup:
    add rsp, SHADOW_SPACE + 16
    pop rbp
    ret

;------------------------------------------------------------------------------
; handle_hello_message: Process HELLO opcode message
; Input: ws_message contains the HELLO message
; Output: RAX = result code
;------------------------------------------------------------------------------
handle_hello_message:
    push rbp
    mov rbp, rsp
    sub rsp, SHADOW_SPACE + 16
    
    ; Parse heartbeat interval from HELLO message
    mov rax, [ws_message + WS_MESSAGE_DATA_OFFSET]
    
%ifdef WINDOWS
    mov rcx, rax                   ; JSON data
    lea rdx, [rbp-4]              ; Heartbeat interval output
%else
    mov rdi, rax                   ; JSON data  
    lea rsi, [rbp-4]              ; Heartbeat interval output
%endif
    call discord_json_parse_hello
    
    test rax, rax
    jnz .parse_failed
    
    ; Store heartbeat interval
    mov eax, [rbp-4]
    mov [heartbeat_interval], eax
    
    ; Send IDENTIFY message
    call send_identify_message
    test rax, rax
    jnz .identify_failed
    
    ; Set up heartbeat timer
    call discord_time_now_ms
    mov [last_heartbeat], rax
    
    mov rax, DISCORD_OK
    jmp .cleanup

.parse_failed:
.identify_failed:
    ; RAX already contains error code
    
.cleanup:
    add rsp, SHADOW_SPACE + 16
    pop rbp
    ret

;------------------------------------------------------------------------------
; handle_heartbeat_ack_message: Process HEARTBEAT_ACK opcode
; Output: RAX = result code  
;------------------------------------------------------------------------------
handle_heartbeat_ack_message:
    ; Simply acknowledge that we received the ACK
    ; In a full implementation, we'd track this for connection health
    mov rax, DISCORD_OK
    ret

;------------------------------------------------------------------------------
; send_identify_message: Send IDENTIFY message to gateway
; Uses the saved bot token from gateway_run
; Output: RAX = result code
;------------------------------------------------------------------------------
send_identify_message:
    push rbp
    mov rbp, rsp
    sub rsp, SHADOW_SPACE + 16
    
    ; Create IDENTIFY JSON
    mov rax, [rbp-8]               ; Get saved token from parent frame
    test rax, rax
    jz .no_token
    
%ifdef WINDOWS
    mov rcx, rax                   ; Token parameter
    lea rdx, [rbp-8]              ; JSON output pointer
%else
    mov rdi, rax                   ; Token parameter
    lea rsi, [rbp-8]              ; JSON output pointer
%endif
    call discord_json_create_identify
    
    test rax, rax
    jnz .json_failed
    
    ; Send the IDENTIFY message
    mov rax, [rbp-8]               ; Get created JSON
    
%ifdef WINDOWS
    mov rcx, [gateway_ptr]         ; Gateway
    mov rdx, rax                   ; JSON data
    mov r8, -1                     ; Let strlen calculate length
%else
    mov rdi, [gateway_ptr]         ; Gateway
    mov rsi, rax                   ; JSON data
    ; Calculate length
    push rax
    call strlen                    ; Assume strlen is available or implement inline
    mov rdx, rax                   ; Length
    pop rsi                        ; Restore JSON data pointer
%endif
    
    ; For simplicity, calculate length inline (basic strlen)
    push rsi
    mov rcx, 0
.strlen_loop:
    cmp byte [rsi + rcx], 0
    je .strlen_done
    inc rcx
    jmp .strlen_loop
.strlen_done:
    mov rdx, rcx                   ; Length in RDX
    pop rsi                        ; Restore JSON data
    
    call discord_ws_send
    push rax                       ; Save send result
    
    ; Free the JSON
%ifdef WINDOWS
    mov rcx, [rbp-8]
%else
    mov rdi, [rbp-8]
%endif
    call discord_json_free
    
    pop rax                        ; Restore send result
    jmp .cleanup

.no_token:
.json_failed:
    ; RAX contains error code
    
.cleanup:
    add rsp, SHADOW_SPACE + 16
    pop rbp
    ret

;------------------------------------------------------------------------------
; check_and_send_heartbeat: Check if heartbeat is due and send if needed
; Output: RAX = result code
;------------------------------------------------------------------------------
check_and_send_heartbeat:
    push rbp
    mov rbp, rsp
    sub rsp, SHADOW_SPACE + 16
    
    ; Check if heartbeat interval is set
    mov eax, [heartbeat_interval]
    test eax, eax
    jz .no_heartbeat_needed
    
    ; Get current time
    call discord_time_now_ms
    mov rbx, rax                   ; Current time in RBX
    
    ; Calculate when next heartbeat is due
    mov rax, [last_heartbeat]
    mov ecx, [heartbeat_interval]
    add rax, rcx                   ; last_heartbeat + interval
    
    ; Check if heartbeat is due
    cmp rbx, rax
    jl .no_heartbeat_needed
    
    ; Send heartbeat
    mov eax, [sequence_number]
    
%ifdef WINDOWS
    mov ecx, eax                   ; Sequence number
    lea rdx, [rbp-8]              ; JSON output
%else
    mov edi, eax                   ; Sequence number
    lea rsi, [rbp-8]              ; JSON output
%endif
    call discord_json_create_heartbeat
    
    test rax, rax
    jnz .heartbeat_failed
    
    ; Send heartbeat message
    mov rax, [rbp-8]               ; JSON data
    
%ifdef WINDOWS
    mov rcx, [gateway_ptr]         ; Gateway
    mov rdx, rax                   ; JSON data
%else
    mov rdi, [gateway_ptr]         ; Gateway
    mov rsi, rax                   ; JSON data
%endif
    
    ; Calculate length (inline strlen again)
    push rsi
    mov rcx, 0
.hb_strlen_loop:
    cmp byte [rsi + rcx], 0
    je .hb_strlen_done
    inc rcx
    jmp .hb_strlen_loop
.hb_strlen_done:
    mov rdx, rcx
    pop rsi
    
    call discord_ws_send
    push rax                       ; Save result
    
    ; Free JSON
%ifdef WINDOWS
    mov rcx, [rbp-8]
%else
    mov rdi, [rbp-8]
%endif
    call discord_json_free
    
    ; Update last heartbeat time
    call discord_time_now_ms
    mov [last_heartbeat], rax
    
    pop rax                        ; Restore send result
    jmp .cleanup

.no_heartbeat_needed:
    mov rax, DISCORD_OK

.heartbeat_failed:
.cleanup:
    add rsp, SHADOW_SPACE + 16
    pop rbp
    ret

;------------------------------------------------------------------------------
; discord_gateway_disconnect: Disconnect from Discord Gateway
; Output: RAX = result code
;------------------------------------------------------------------------------
discord_gateway_disconnect:
    push rbp
    mov rbp, rsp
    sub rsp, SHADOW_SPACE
    
    ; Check if gateway is connected
    mov rax, [gateway_ptr]
    test rax, rax
    jz .not_connected
    
    ; Close WebSocket connection
%ifdef WINDOWS
    mov rcx, [gateway_ptr]
%else
    mov rdi, [gateway_ptr]
%endif
    call discord_ws_close
    
    ; Clear gateway pointer
    mov qword [gateway_ptr], 0
    
    ; Reset state
    mov dword [heartbeat_interval], 0
    mov qword [last_heartbeat], 0
    mov dword [sequence_number], -1
    mov byte [is_ready], 0
    
    mov rax, DISCORD_OK
    jmp .cleanup

.not_connected:
    mov rax, DISCORD_OK            ; Already disconnected

.cleanup:
    add rsp, SHADOW_SPACE
    pop rbp
    ret