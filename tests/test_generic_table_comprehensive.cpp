#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <memory>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <WinKernelLite/ntrtl.h>

// switch to using AVL tables
#define RTL_USE_AVL_TABLES 0

class GenericTableComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

typedef struct _CONNECTION_INFO {
    ULONGLONG   connectionTime;
    BYTE        address[16];
	UINT        addressSize; // 4 for IPv4, 16 for IPv6
} CONNECTION_INFO, *PCONNECTION_INFO;

   // Global memory tracking for testing
static size_t g_allocationCount = 0;
static size_t g_freeCount = 0;
static size_t g_bytesAllocated = 0;

RTL_GENERIC_TABLE           gOutStandingConnections;

// Test allocation routine with tracking
PVOID NTAPI AllocateConnectionInfo(PRTL_GENERIC_TABLE Table, LONG Size) {
    UNREFERENCED_PARAMETER(Table);
    g_allocationCount++;
    g_bytesAllocated += Size;
    return malloc(Size);
}


// Test free routine with tracking
VOID NTAPI FreeConnectionInfo(PRTL_GENERIC_TABLE Table, PVOID Buffer) {
    UNREFERENCED_PARAMETER(Table);
    if (Buffer) {
        g_freeCount++;
        free(Buffer);
    }
}


RTL_GENERIC_COMPARE_RESULTS
NTAPI
CompareConnectionInfo(
    IN  struct _RTL_GENERIC_TABLE  *Table,
    IN  PVOID                       FirstElement,
    IN  PVOID                       SecondElement
)
{
    PCONNECTION_INFO firstElement, secondElement;
    INT rc;

    UNREFERENCED_PARAMETER(Table);

    firstElement = (PCONNECTION_INFO)FirstElement;
    secondElement = (PCONNECTION_INFO)SecondElement;

	// just compare the binary address
    rc = memcmp(firstElement->address, secondElement->address, firstElement->addressSize);
    return (rc < 0) ? GenericLessThan :
           (rc > 0) ? GenericGreaterThan :
                      GenericEqual;
}


TEST_F(GenericTableComprehensiveTest, MyNewTest) {

    RtlInitializeGenericTable( &gOutStandingConnections,
                               CompareConnectionInfo,
                               AllocateConnectionInfo,
                               FreeConnectionInfo,
                               NULL );

    BOOLEAN bNewElement;
    ULONGLONG currentTime;
    GetSystemTimeAsFileTime((LPFILETIME)&currentTime);

    // Generate 10 different IPv4 addresses and convert them to binary format
    std::vector<std::string> ipv4Addresses = {
        "192.168.1.1",
        "10.0.0.1",
        "172.16.0.1",
        "8.8.8.8",
        "1.1.1.1",
        "192.168.0.100",
        "10.10.10.10",
        "172.31.255.254",
        "208.67.222.222",
        "4.4.4.4"
    };

    for (const auto& ipStr : ipv4Addresses) {
        PCONNECTION_INFO el = (PCONNECTION_INFO)malloc(sizeof(CONNECTION_INFO));
        if (!el) {
            continue; // Skip if allocation fails
        }

        // Zero out the structure
        memset(el, 0, sizeof(CONNECTION_INFO));
        
        el->connectionTime = currentTime;
        el->addressSize = 4; // IPv4

        struct sockaddr_in addr4;
        memset(&addr4, 0, sizeof(addr4));
        addr4.sin_family = AF_INET;

        // convert string IP address to binary
        int result = inet_pton(AF_INET, ipStr.c_str(), &addr4.sin_addr);
        if (result == 1) {
            // Successfully converted, copy the binary address
            if (AF_INET == addr4.sin_family) {
                el->addressSize = 4;
                memcpy(el->address, &addr4.sin_addr.s_addr, 4);
            }

            // Insert into generic table
            auto res = RtlInsertElementGenericTable(&gOutStandingConnections, el, sizeof(*el), &bNewElement);
            
            if (!res) {
                free(el);
            } else {
                // Verify the insertion was successful
                EXPECT_TRUE(bNewElement) << "Failed to insert new element for IP: " << ipStr;
                
                // Verify we can look up the element
                PCONNECTION_INFO found = (PCONNECTION_INFO)RtlLookupElementGenericTable(&gOutStandingConnections, el);
                EXPECT_NE(found, nullptr) << "Failed to lookup inserted element for IP: " << ipStr;
                
                if (found) {
                    EXPECT_EQ(found->addressSize, 4);
                    EXPECT_EQ(memcmp(found->address, &addr4.sin_addr.s_addr, 4), 0) << "Address mismatch for IP: " << ipStr;
                }
            }
        } else {
            // Failed to convert IP address
            free(el);
            FAIL() << "Failed to convert IP address: " << ipStr;
        }
    }

    // Verify we have all 10 elements in the table
    EXPECT_EQ(RtlNumberGenericTableElements(&gOutStandingConnections), 10UL);

    // Clean up - enumerate and verify all elements
    ULONG elementCount = 0;
    PVOID element = RtlEnumerateGenericTable(&gOutStandingConnections, TRUE);
    while (element != nullptr) {
        elementCount++;
        PCONNECTION_INFO connInfo = (PCONNECTION_INFO)element;
        EXPECT_EQ(connInfo->addressSize, 4);
        EXPECT_EQ(connInfo->connectionTime, currentTime);
        
        element = RtlEnumerateGenericTable(&gOutStandingConnections, FALSE);
    }
    
    EXPECT_EQ(elementCount, 10UL) << "Enumeration found different number of elements than expected";
}
