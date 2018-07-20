#include "interface.h"
#include "util.h"

#include <libvirt/libvirt.h>

static virInterfacePtr
virtDBusInterfaceGetVirInterface(virtDBusConnect *connect,
                                 const gchar *objectPath,
                                 GError **error)
{
    virInterfacePtr interface;

    if (virtDBusConnectOpen(connect, error) < 0)
        return NULL;

    interface = virtDBusUtilVirInterfaceFromBusPath(connect->connection,
                                                    objectPath,
                                                    connect->interfacePath);
    if (!interface) {
        virtDBusUtilSetLastVirtError(error);
        return NULL;
    }

    return interface;
}

static void
virtDBusInterfaceCreate(GVariant *inArgs,
                        GUnixFDList *inFDs G_GNUC_UNUSED,
                        const gchar *objectPath,
                        gpointer userData,
                        GVariant **outArgs G_GNUC_UNUSED,
                        GUnixFDList **outFDs G_GNUC_UNUSED,
                        GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virInterface) interface = NULL;
    guint flags;

    g_variant_get(inArgs, "(u)", &flags);

    interface = virtDBusInterfaceGetVirInterface(connect, objectPath, error);
    if (!interface)
        return;

    if (virInterfaceCreate(interface, flags) < 0)
        virtDBusUtilSetLastVirtError(error);
}

static void
virtDBusInterfaceDestroy(GVariant *inArgs,
                         GUnixFDList *inFDs G_GNUC_UNUSED,
                         const gchar *objectPath,
                         gpointer userData,
                         GVariant **outArgs G_GNUC_UNUSED,
                         GUnixFDList **outFDs G_GNUC_UNUSED,
                         GError **error)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virInterface) interface = NULL;
    guint flags;

    g_variant_get(inArgs, "(u)", &flags);

    interface = virtDBusInterfaceGetVirInterface(connect, objectPath, error);
    if (!interface)
        return;

    if (virInterfaceDestroy(interface, flags) < 0)
        virtDBusUtilSetLastVirtError(error);
}

static virtDBusGDBusPropertyTable virtDBusInterfacePropertyTable[] = {
    { 0 }
};

static virtDBusGDBusMethodTable virtDBusInterfaceMethodTable[] = {
    { "Create", virtDBusInterfaceCreate },
    { "Destroy", virtDBusInterfaceDestroy },
    { 0 }
};

static gchar **
virtDBusInterfaceEnumerate(gpointer userData)
{
    virtDBusConnect *connect = userData;
    g_autoptr(virInterfacePtr) interfaces = NULL;
    gint num = 0;
    gchar **ret = NULL;

    if (!virtDBusConnectOpen(connect, NULL))
        return NULL;

    num = virConnectListAllInterfaces(connect->connection, &interfaces, 0);
    if (num < 0)
        return NULL;

    if (num == 0)
        return NULL;

    ret = g_new0(gchar *, num + 1);

    for (gint i = 0; i < num; i++) {
        ret[i] = virtDBusUtilBusPathForVirInterface(interfaces[i],
                                                    connect->interfacePath);
    }

    return ret;
}

static GDBusInterfaceInfo *interfaceInfo;

void
virtDBusInterfaceRegister(virtDBusConnect *connect,
                          GError **error)
{
    connect->interfacePath = g_strdup_printf("%s/interface",
                                             connect->connectPath);

    if (!interfaceInfo) {
        interfaceInfo = virtDBusGDBusLoadIntrospectData(VIRT_DBUS_INTERFACE_INTERFACE,
                                                        error);
        if (!interfaceInfo)
            return;
    }

    virtDBusGDBusRegisterSubtree(connect->bus,
                                 connect->interfacePath,
                                 interfaceInfo,
                                 virtDBusInterfaceEnumerate,
                                 virtDBusInterfaceMethodTable,
                                 virtDBusInterfacePropertyTable,
                                 connect);
}
