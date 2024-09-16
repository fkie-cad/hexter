#pragma once

typedef struct _MODULE_INFO {
    PVOID Base;
    ULONG Size;
} MODULE_INFO, *PMODULE_INFO;


/**
 * 
 */
INT AddPrivileges(
    PCHAR *Privileges,
    UINT32 PrivilegeCount
)
{
    INT s = 0;
    HANDLE htoken;
    ULONG i;

    TOKEN_PRIVILEGES* p = NULL;

    if ( OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &htoken) )
    {
        size_t htokenSize = sizeof(TOKEN_PRIVILEGES) + (PrivilegeCount-1) * sizeof(LUID_AND_ATTRIBUTES);
        p = (PTOKEN_PRIVILEGES)malloc(htokenSize);
        if ( !p )
        {
            s = GetLastError();
            goto clean;
        }

        for ( i = 0; i < PrivilegeCount; i++ )
        {
            if ( !LookupPrivilegeValueA(NULL, Privileges[i], &(p->Privileges[i].Luid)) )
            {
                s = GetLastError();
                goto clean;
            }

            p->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
        }
        p->PrivilegeCount = PrivilegeCount;

        if ( !AdjustTokenPrivileges(htoken, FALSE, p, (ULONG)htokenSize, NULL, NULL) 
           || GetLastError() != ERROR_SUCCESS )
        {
            s = GetLastError();
            goto clean;
        }
    }
    else
    {
        s = GetLastError();
        goto clean;
    }

clean:
    if ( p )
        free(p);

    return s;
}

BOOL IsProcessElevated()
{
    BOOL fIsElevated = FALSE;
    HANDLE hToken = NULL;
    TOKEN_ELEVATION elevation;
    DWORD dwSize;

    if ( !OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) )
    {
        goto clean;  // if Failed, we treat as False
    }

    if (!GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
    {   
        goto clean;// if Failed, we treat as False
    }

    fIsElevated = elevation.TokenIsElevated;

clean:
    if (hToken)
    {
        CloseHandle(hToken);
        hToken = NULL;
    }
    return fIsElevated; 
}
