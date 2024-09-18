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
    UINT32 PrivilegesCount
)
{
    INT s = 0;
    HANDLE token;
    ULONG i;

    TOKEN_PRIVILEGES* tp = NULL;

    if ( OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token) )
    {
        size_t tpSize = sizeof(TOKEN_PRIVILEGES) + (PrivilegesCount-1) * sizeof(LUID_AND_ATTRIBUTES);
        tp = (PTOKEN_PRIVILEGES)malloc(tpSize);
        if ( !tp )
        {
            s = GetLastError();
            goto clean;
        }

        for ( i = 0; i < PrivilegesCount; i++ )
        {
            if ( !LookupPrivilegeValueA(NULL, Privileges[i], &(tp->Privileges[i].Luid)) )
            {
                s = GetLastError();
                goto clean;
            }

            tp->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
        }
        tp->PrivilegeCount = PrivilegesCount;

        if ( !AdjustTokenPrivileges(token, FALSE, tp, (ULONG)tpSize, NULL, NULL) 
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
    if ( tp )
        free(tp);

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
        goto clean;
    }

    if ( !GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize) )
    {   
        goto clean;
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
