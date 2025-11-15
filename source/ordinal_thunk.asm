.code
extern OrdinalProc1 : qword
extern OrdinalProc2 : qword
extern OrdinalProc3 : qword
extern OrdinalProc4 : qword
extern OrdinalProc5 : qword
extern OrdinalProc6 : qword

OrdinalThunk1 proc
    jmp qword ptr [OrdinalProc1]
OrdinalThunk1 endp

OrdinalThunk2 proc
    jmp qword ptr [OrdinalProc2]
OrdinalThunk2 endp

OrdinalThunk3 proc
    jmp qword ptr [OrdinalProc3]
OrdinalThunk3 endp

OrdinalThunk4 proc
    jmp qword ptr [OrdinalProc4]
OrdinalThunk4 endp

OrdinalThunk5 proc
    jmp qword ptr [OrdinalProc5]
OrdinalThunk5 endp

OrdinalThunk6 proc
    jmp qword ptr [OrdinalProc6]
OrdinalThunk6 endp

end
