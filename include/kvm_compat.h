#ifndef __LIBKVMI_KVM_COMPAT_H__
#define __LIBKVMI_KVM_COMPAT_H__

/*
 * kvm_para.h
 */

#ifndef KVM_EAGAIN
# define KVM_EAGAIN 11
#endif

#ifndef KVM_EBUSY
# define KVM_EBUSY EBUSY
#endif

#ifndef KVM_ENOENT
# define KVM_ENOENT ENOENT
#endif

#ifndef KVM_ENOMEM
# define KVM_ENOMEM ENOMEM
#endif

#ifndef KVM_EINVAL
# define KVM_EINVAL EINVAL
#endif

#ifndef KVM_EOPNOTSUPP
# define KVM_EOPNOTSUPP 95
#endif

#endif
